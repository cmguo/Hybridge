#include "signalhandler.h"
#include "publisher.h"
#include "collection.h"
#include "value.h"
#include "bridge.h"
#include "debug.h"

SignalHandler::SignalHandler(Publisher *receiver)
    : m_receiver(receiver)
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
inline MetaMethod const & findSignal(const MetaObject *metaObject, const int signalIndex)
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

void SignalHandler::connectTo(const Object *object, const int signalIndex)
{
    const MetaObject *metaObject = m_receiver->bridge_->metaObject(object);
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
    MetaObject::Connection connection = m_receiver->bridge_->connect(object, signal.methodIndex());
    if (!connection) {
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
    for (int i = 0; i < signal.parameterCount(); ++i) {
        int tp = signal.parameterType(i);
//        if (tp == QMetaType::UnknownType) {
//            warning("Don't know how to handle '%s', use qRegisterMetaType to register it.",
//                    signal.parameterName(i));
//        }
        args.emplace_back(tp);
    }

    m_signalArgumentTypes[metaObject][signal.methodIndex()] = args;
}

void SignalHandler::dispatch(const Object *object, const int signalIdx, Array && arguments)
{
    const MetaObject *metaObject = m_receiver->bridge_->metaObject(object);
    assert(mapContains(m_signalArgumentTypes, metaObject));
    const std::unordered_map<int, std::vector<int> > &objectSignalArgumentTypes = mapValue(m_signalArgumentTypes, metaObject);
    std::unordered_map<int, std::vector<int> >::const_iterator signalIt = objectSignalArgumentTypes.find(signalIdx);
    if (signalIt == objectSignalArgumentTypes.cend()) {
        // not connected to this signal, skip
        return;
    }
//    const std::vector<int> &argumentTypes = signalIt->second;
//    Array arguments;
//    arguments.reserve(argumentTypes.size());
    // TODO: basic overload resolution based on number of arguments?
//    for (int i = 0; i < argumentTypes.size(); ++i) {
//        const QMetaType::Type type = static_cast<QMetaType::Type>(argumentTypes.at(i));
//        MsgValue arg;
//        if (type == QMetaType::QVariant) {
//            arg = *reinterpret_cast<QVariant *>(argumentData[i + 1]);
//        } else {
//            arg = QVariant(type, argumentData[i + 1]);
//        }
//        arguments.append(arg);
//    }
    m_receiver->signalEmitted(object, signalIdx, std::move(arguments));
}

void SignalHandler::disconnectFrom(const Object *object, const int signalIndex)
{
    assert(mapContains(mapValue(m_connectionsCounter, object), signalIndex));
    ConnectionPair &connection = m_connectionsCounter[object][signalIndex];
    --connection.second;
    if (!connection.second || !connection.first) {
        m_receiver->bridge_->disconnect(connection.first);
        m_connectionsCounter[object].erase(signalIndex);
        if (m_connectionsCounter[object].empty()) {
            m_connectionsCounter.erase(object);
        }
    }
}

//int SignalHandler::qt_metacall(MetaObject::Call call, int methodId, void **args)
//{
//    methodId = Object::qt_metacall(call, methodId, args);
//    if (methodId < 0)
//        return methodId;

//    if (call == MetaObject::InvokeMetaMethod) {
//        const Object *object = sender();
//        assert(object);
//        assert(senderSignalIndex() == methodId);
//        assert(m_connectionsCounter.contains(object));
//        assert(m_connectionsCounter.value(object).contains(methodId));

//        dispatch(object, methodId, args);

//        return -1;
//    }
//    return methodId;
//}

void SignalHandler::clear()
{
    for (auto &connections : m_connectionsCounter) {
        for (auto &connection : connections.second) {
            m_receiver->bridge_->disconnect(connection.second.first);
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
    const SignalConnectionHash &connections = mapValue(m_connectionsCounter, object);
    for (auto &connection : connections) {
        m_receiver->bridge_->disconnect(connection.second.first);
    }
    m_connectionsCounter.erase(object);
}
