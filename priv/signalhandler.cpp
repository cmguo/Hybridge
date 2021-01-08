#include "signalhandler.h"
#include "publisher.h"
#include "collection.h"
#include "core/value.h"
#include "core/channel.h"
#include "debug.h"

SignalHandler::SignalHandler(Publisher *receiver)
    : publisher_(receiver)
{
    // we must know the arguments of a destroyed signal for the global static meta object of Object
    // otherwise, we might end up with missing m_signalArgumentTypes information in dispatch
    //setupSignalArgumentTypes(&Object::staticMetaObject, Object::staticMetaObject.method(s_destroyedSignalIndex));
}

/**
 * Find and return the signal of index @p signalIndex in the meta object of @p object and return it.
 *
 * The return value is also verified to ensure it is a signal.
 */
inline MetaMethod const & findSignal(const MetaObject *metaObject, size_t signalIndex)
{
    MetaMethod const & signal = metaObject->method(signalIndex);
    if (!signal.isValid()) {
        warning("Cannot find signal with index %d of object %s", signalIndex, metaObject->className());
        static EmptyMetaMethod m;
        return m;
    }
    assert(signal.isSignal());
    return signal;
}

static void dispatchSignal(void * handler, Object const * object, size_t index, Array && args)
{
    reinterpret_cast<SignalHandler*>(handler)->dispatch(object, index, std::move(args));
}

void SignalHandler::connectTo(const Object *object, size_t signalIndex)
{
    const MetaObject *metaObject = publisher_->channel_->metaObject(object);
    const MetaMethod &signal = findSignal(metaObject, signalIndex);
    if (!signal.isValid()) {
        return;
    }

    ConnectionPair &connectionCounter = m_connectionsCounter[object][signalIndex];
    if (connectionCounter.first) {
        // increase connection counter if already connected
        ++connectionCounter.second;
        return;
    } // otherwise not yet connected, do so now

    //static const int memberOffset = Object::staticMetaObject.methodCount();
    MetaObject::Connection connection(object, signal.methodIndex(), this, &dispatchSignal);
    if (!metaObject->connect(connection)) {
        warning("SignalHandler: MetaObject::connect returned false. Unable to connect to", object, signal.name(), signal.methodSignature());
        return;
    }
    connectionCounter.first = connection;
    connectionCounter.second = 1;

    setupSignalArgumentTypes(metaObject, signal);
}

void SignalHandler::setupSignalArgumentTypes(const MetaObject *metaObject, const MetaMethod &signal)
{
    if (mapContains(mapValue(m_signalArgumentTypes, metaObject), signal.methodIndex())) {
        return;
    }
    // find the type ids of the signal parameters, see also QSignalSpy::initArgs
    std::vector<int> args;
    args.reserve(static_cast<size_t>(signal.parameterCount()));
    for (size_t i = 0; i < signal.parameterCount(); ++i) {
        int tp = signal.parameterType(i);
        if (tp == Value::None) {
            warning("Don't know how to handle '%s', use qRegisterMetaType to register it.",
                    signal.parameterName(i));
        }
        args.emplace_back(tp);
    }

    m_signalArgumentTypes[metaObject][signal.methodIndex()] = args;
}

void SignalHandler::dispatch(const Object *object, size_t signalIdx, Array && arguments)
{
    const MetaObject *metaObject = publisher_->channel_->metaObject(object);
    assert(mapContains(m_signalArgumentTypes, metaObject));
    const std::unordered_map<size_t, std::vector<int> > &objectSignalArgumentTypes = mapValue(m_signalArgumentTypes, metaObject);
    std::unordered_map<size_t, std::vector<int> >::const_iterator signalIt = objectSignalArgumentTypes.find(signalIdx);
    if (signalIt == objectSignalArgumentTypes.cend()) {
        // not connected to this signal, skip
        return;
    }
    publisher_->signalEmitted(object, signalIdx, std::move(arguments));
}

void SignalHandler::disconnectFrom(const Object *object, size_t signalIndex)
{
    assert(mapContains(mapValue(m_connectionsCounter, object), signalIndex));
    ConnectionPair &connection = m_connectionsCounter[object][signalIndex];
    --connection.second;
    if (!connection.second || !connection.first) {
        const MetaObject *metaObject = publisher_->channel_->metaObject(object);
        metaObject->disconnect(connection.first);
        m_connectionsCounter[object].erase(signalIndex);
        if (m_connectionsCounter[object].empty()) {
            m_connectionsCounter.erase(object);
        }
    }
}

void SignalHandler::clear()
{
    for (auto &connections : m_connectionsCounter) {
        const MetaObject *metaObject = publisher_->channel_->metaObject(
                    connections.first);
        for (auto &connection : connections.second) {
            metaObject->disconnect(connection.second.first);
        }
    }
    m_connectionsCounter.clear();
    //const SignalArgumentHash keep = m_signalArgumentTypes.take(&Object::staticMetaObject);
    m_signalArgumentTypes.clear();
    //m_signalArgumentTypes[&Object::staticMetaObject] = keep;
}

void SignalHandler::remove(const Object *object)
{
    assert(mapContains(m_connectionsCounter, object));
    const MetaObject *metaObject = publisher_->channel_->metaObject(object);
    const SignalConnectionHash &connections = mapValue(m_connectionsCounter, object);
    for (auto &connection : connections) {
        metaObject->disconnect(connection.second.first);
    }
    m_connectionsCounter.erase(object);
}
