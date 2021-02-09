#include "publisher.h"
#include "core/channel.h"
#include "core/transport.h"
#include "core/metaobject.h"
#include "core/channel.h"
#include "collection.h"
#include "core/value.h"
#include "debug.h"

#include <iostream>

namespace {


    Message createResponse(Value &&id, Value && data)
    {
        Message response;
        response[KEY_TYPE] = TypeResponse;
        response[KEY_ID] = std::move(id);
        response[KEY_DATA] = std::move(data);
        return response;
    }

    /// TODO: what is the proper value here?
    const int PROPERTY_UPDATE_INTERVAL = 50;
}

Publisher::Publisher(Channel * bridge)
    : channel_(bridge)
    , signalHandler_(this)
    , clientIsIdle_(false)
    , blockUpdates_(false)
    , propertyUpdatesInitialized_(false)
{
}

Publisher::~Publisher()
{
}

void Publisher::registerObject(std::string const &id, Object *object)
{
    registeredObjects_[id] = object;
    objectIds_[object] = id;
    if (propertyUpdatesInitialized_) {
        if (!channel_->transports_.empty()) {
            warning("Registered new object after initialization, existing clients won't be notified!");
            // TODO: send a message to clients that an object was added
        }
        initializePropertyUpdates(object, classInfoForObject(object, nullptr));
    }
}

Map Publisher::classInfoForObject(const Object *object, Transport *transport)
{
    Map data;
    if (!object) {
        warning("null object given to MetaObjectPublisher - bad API usage?");
        return data;
    }

    Array signals;
    Array methods;
    Array properties;
    Map qtEnums;

    const MetaObject *metaObject = channel_->metaObject(object);
    std::set<size_t> notifySignals;
    std::set<std::string > identifiers;
    for (size_t i = 0; i < metaObject->propertyCount(); ++i) {
        const MetaProperty &prop = metaObject->property(i);
        Array propertyInfo;
        const std::string &propertyName = prop.name();
        propertyInfo.emplace_back(static_cast<int>(prop.propertyIndex()));
        propertyInfo.emplace_back(propertyName);
        identifiers.emplace(propertyName);
        Array signalInfo;
        if (prop.hasNotifySignal()) {
            notifySignals.emplace(prop.notifySignalIndex());
            // optimize: compress the common propertyChanged notification names, just send a 1
            const std::string  &notifySignal = prop.notifySignal().name();
            static const std::string  changedSuffix = "Changed";
            if (notifySignal == propertyName + changedSuffix)
            {
                signalInfo.emplace_back(1);
            } else {
                signalInfo.emplace_back(notifySignal);
            }
            signalInfo.emplace_back(static_cast<int>(prop.notifySignalIndex()));
        } else if (!prop.isConstant()) {
            warning("Property '%s'' of object '%s' has no notify signal and is not constant, "
                     "value updates in HTML will be broken!",
                     prop.name(), metaObject->className());
        }
        propertyInfo.emplace_back(std::move(signalInfo));
        propertyInfo.emplace_back(wrapResult(prop.read(object), transport));
        std::cout << "property: " << propertyName << " = " << propertyInfo.back() << std::endl;
        properties.emplace_back(std::move(propertyInfo));
    }
    for (size_t i = 0; i < metaObject->methodCount(); ++i) {
        if (contains(notifySignals, i)) {
            continue;
        }
        const MetaMethod &method = metaObject->method(i);
        //NOTE: this must be a string, otherwise it will be converted to '{}' in QML
        const std::string &name = method.name();
        // optimize: skip overloaded methods/signals or property getters, on the JS side we can only
        // call one of them anyways
        // TODO: basic support for overloaded signals, methods
        if (contains(identifiers, name)) {
            continue;
        }
        identifiers.emplace(name);
        // send data as array to client with format: [name, index]
        Array data;
        data.emplace_back(name);
        data.emplace_back(static_cast<int>(i));
        data.emplace_back(static_cast<int>(method.returnType()));
        Array paramTypes;
        Array paramNames;
        for (size_t j = 0; j < method.parameterCount(); ++j) {
            paramTypes.emplace_back(method.parameterType(j));
            paramNames.emplace_back(std::string(method.parameterName(j)));
        }
        data.emplace_back(std::move(paramTypes));
        data.emplace_back(std::move(paramNames));
        if (method.isSignal()) {
            signals.emplace_back(std::move(data));
        } else if (method.isPublic()) {
            methods.emplace_back(std::move(data));
        }
    }
    for (size_t i = 0; i < metaObject->enumeratorCount(); ++i) {
        MetaEnum const & enumerator = metaObject->enumerator(i);
        Map values;
        for (size_t k = 0; k < enumerator.keyCount(); ++k) {
            values[enumerator.key(k)] = enumerator.value(k);
        }
        qtEnums[enumerator.name()] = std::move(values);
    }
    data[KEY_CLASS] = std::string(metaObject->className());
    data[KEY_SIGNALS] = std::move(signals);
    data[KEY_METHODS] = std::move(methods);
    data[KEY_PROPERTIES] = std::move(properties);
    if (!qtEnums.empty()) {
        data[KEY_ENUMS] = std::move(qtEnums);
    }
    return data;
}

void Publisher::setClientIsIdle(bool isIdle)
{
    if (clientIsIdle_ == isIdle) {
        return;
    }
    clientIsIdle_ = isIdle;
    if (!isIdle) {
        channel_->stopTimer();
    } else {
        channel_->startTimer(PROPERTY_UPDATE_INTERVAL);
    }
}

Map Publisher::initializeClient(Transport *transport)
{
    Map objectInfos;
    {
        const std::unordered_map<std::string, Object *>::const_iterator end = registeredObjects_.cend();
        for (std::unordered_map<std::string, Object *>::const_iterator it = registeredObjects_.cbegin(); it != end; ++it) {
            Map && info = classInfoForObject(it->second, transport);
            if (!propertyUpdatesInitialized_) {
                initializePropertyUpdates(it->second, info);
            }
            objectInfos[it->first] = std::move(info);
        }
    }
    propertyUpdatesInitialized_ = true;
    return objectInfos;
}

void Publisher::initializePropertyUpdates(const Object *const object, const Map &objectInfo)
{
    for (auto & propertyInfoVar : mapValue(objectInfo, KEY_PROPERTIES).toArray()) {
        const Array &propertyInfo = propertyInfoVar.toArray();
        if (propertyInfo.size() < 2) {
            warning("Invalid property info encountered:", propertyInfoVar);
            continue;
        }
        size_t propertyIndex = static_cast<size_t>(propertyInfo[0].toInt(0));
        const Array &signalData = propertyInfo.at(2).toArray();

        if (signalData.empty()) {
            // Property without NOTIFY signal
            continue;
        }

        size_t signalIndex = static_cast<size_t>(signalData.at(1).toInt());

        std::set<size_t> &connectedProperties = signalToPropertyMap_[object][signalIndex];

        // Only connect for a property update once
        if (connectedProperties.empty()) {
            signalHandler_.connectTo(object, signalIndex);
        }

        connectedProperties.insert(propertyIndex);
    }

    // also always connect to destroyed signal
    signalHandler_.connectTo(object, 0);
}

void Publisher::sendPendingPropertyUpdates()
{
    if (blockUpdates_ || !clientIsIdle_
            || (pendingPropertyUpdates_.empty() && pendingPropertyUpdates2_.empty())) {
        return;
    }

    Array data;
    Array data2;
    std::map<Transport*, Array> specificUpdates;

    // convert pending property updates to JSON data
    const PendingPropertyUpdates::const_iterator end = pendingPropertyUpdates_.cend();
    for (PendingPropertyUpdates::const_iterator it = pendingPropertyUpdates_.cbegin(); it != end; ++it) {
        const Object *object = it->first;
        const MetaObject *const metaObject = channel_->metaObject(object);
        const std::string objectId = mapValue(objectIds_, object);
        const SignalToPropertyNameMap &objectssignalToPropertyMap_ = mapValue(signalToPropertyMap_, object);
        // maps property name to current property value
        Map properties;
        // maps signal index to list of arguments of the last emit
        Map sigs;
        const SignalToArgumentsMap::const_iterator sigEnd = it->second.cend();
        for (SignalToArgumentsMap::const_iterator sigIt = it->second.cbegin(); sigIt != sigEnd; ++sigIt) {
            // TODO: can we get rid of the int <-> string conversions here?
            for (size_t propertyIndex : mapValue(objectssignalToPropertyMap_, sigIt->first)) {
                const MetaProperty &property = metaObject->property(propertyIndex);
                assert(property.isValid());
                Value && v = wrapResult(property.read(object), nullptr, objectId);
                properties[stringNumber(propertyIndex)] = std::move(v);
            }
            sigs[stringNumber(sigIt->first)] = sigIt->second.ref();
        }
        Map obj;
        obj[KEY_OBJECT] = std::move(objectId);
        obj[KEY_SIGNALS] = std::move(sigs);
        obj[KEY_PROPERTIES] = std::move(properties);
        Value obj2(std::move(obj));

        // if the object is auto registered, just send the update only to clients which know this object
        if (mapContains(wrappedObjects_, objectId)) {
            for (Transport *transport : mapValue(wrappedObjects_, objectId).transports) {
                Array &arr = specificUpdates[transport];
                arr.emplace_back(obj2.ref());
            }
            data2.emplace_back(std::move(obj2));
        } else {
            data.emplace_back(std::move(obj2));
        }
    }
    pendingPropertyUpdates_.clear();

    for (auto & it : pendingPropertyUpdates2_) {
        const Object *object = it.first;
        const MetaObject *const metaObject = channel_->metaObject(object);
        const std::string objectId = mapValue(objectIds_, object);
        // maps property name to current property value
        Map properties;
        for (size_t propertyIndex : it.second) {
            // TODO: can we get rid of the int <-> string conversions here?
            const MetaProperty &property = metaObject->property(propertyIndex);
            assert(property.isValid());
            Value && v = wrapResult(property.read(object), nullptr, objectId);
            properties[stringNumber(propertyIndex)] = std::move(v);
        }
        Map obj;
        obj[KEY_OBJECT] = std::move(objectId);
        obj[KEY_PROPERTIES] = std::move(properties);
        Value obj2(std::move(obj));

        // if the object is auto registered, just send the update only to clients which know this object
        if (mapContains(wrappedObjects_, objectId)) {
            for (Transport *transport : mapValue(wrappedObjects_, objectId).transports) {
                Array &arr = specificUpdates[transport];
                arr.emplace_back(obj2.ref());
            }
            data2.emplace_back(std::move(obj2));
        } else {
            data.emplace_back(std::move(obj2));
        }
    }
    pendingPropertyUpdates2_.clear();

    Message message;
    message[KEY_TYPE] = TypePropertyUpdate;

    // data does not contain specific updates
    if (!data.empty()) {
        setClientIsIdle(false);

        message[KEY_DATA] = std::move(data);
        broadcastMessage(message);
    }

    // send every property update which is not supposed to be broadcasted
    std::map<Transport*, Array>::iterator suend = specificUpdates.end();
    for (std::map<Transport*, Array>::iterator it = specificUpdates.begin(); it != suend; ++it) {
        message[KEY_DATA] = std::move(it->second);
        it->first->sendMessage(message);
    }
}

void Publisher::invokeMethod(Object * object, size_t methodIndex, Array &&args, MetaMethod::Response const & resp)
{
    const MetaMethod &method = channel_->metaObject(object)->method(methodIndex);

    if (std::string(method.name()) == "deleteLater") {
        // invoke `deleteLater` on wrapped Object indirectly
        deleteWrappedObject(object);
        return resp(Value());
    } else if (!method.isValid()) {
        warning("Cannot invoke unknown method of index on object.", methodIndex, object);
        return resp(Value());
    } else if (!method.isPublic()) {
        warning("Cannot invoke non-public method on object.", method.name(), object);
        return resp(Value());
    } else if (method.isSignal()) {
        warning("Cannot invoke signal method on object.", method.name(), object);
        return resp(Value());
    } else if (args.size() > method.parameterCount()) {
        warning("Ignoring additional arguments while invoking method on object: arguments given, but method only takes.",
                method.name(), object, method.parameterCount());
    }
    for (size_t i = 0; i < std::min(args.size(), method.parameterCount()); ++i) {
        args[i] = toVariant(std::move(args[i]), method.parameterType(i));
    }
    method.invoke(object, std::move(args), resp);
}

void Publisher::setProperty(Object *object, size_t propertyIndex, Value &&value)
{
    MetaProperty const & property = channel_->metaObject(object)->property(propertyIndex);
    if (!property.isValid()) {
        warning("Cannot set unknown property of object", propertyIndex, object);
    } else if (!property.write(object, toVariant(std::move(value), property.type()))) {
        warning("Could not write value %1 to property %2 of object %3", value, property.name(), object);
    }
}

void Publisher::signalEmitted(const Object *object, size_t signalIndex, Array &&arguments)
{
    if (!channel_ || channel_->transports_.empty()) {
        if (signalIndex == 0)
            objectDestroyed(object);
        return;
    }
    if (!mapContains(mapValue(signalToPropertyMap_, object), signalIndex)) {
        Message message;
        const std::string &objectName = mapValue(objectIds_, object);
        assert(!objectName.empty());
        message[KEY_OBJECT] = objectName;
        message[KEY_SIGNAL] = static_cast<int>(signalIndex);
        if (!arguments.empty()) {
            message[KEY_ARGS] = wrapList(arguments, nullptr, objectName);
        }
        message[KEY_TYPE] = TypeSignal;

        // if the object is wrapped, just send the response to clients which know this object
        if (mapContains(wrappedObjects_, objectName)) {
            for (Transport *transport : mapValue(wrappedObjects_, objectName).transports) {
                transport->sendMessage(message);
            }
        } else {
            broadcastMessage(message);
        }

        if (signalIndex == 0) {
            objectDestroyed(object);
        }
    } else {
        pendingPropertyUpdates_[object][signalIndex] = std::move(arguments);
        if (clientIsIdle_ && !blockUpdates_) {
            channel_->startTimer(PROPERTY_UPDATE_INTERVAL);
        }
    }
}

void Publisher::objectDestroyed(const Object *object)
{
    const std::string &id = mapTake(objectIds_, object);
    assert(!id.empty());
    bool removed = registeredObjects_.erase(id)
            || wrappedObjects_.erase(id);
    assert(removed);
    (void)(removed);

    // only remove from handler when we initialized the property updates
    // cf: https://bugreports.qt.io/browse/QTBUG-60250
    if (propertyUpdatesInitialized_) {
        signalHandler_.remove(object);
        signalToPropertyMap_.erase(object);
    }
    pendingPropertyUpdates_.erase(object);
}

Object *Publisher::unwrapObject(const std::string &objectId) const
{
    if (!objectId.empty()) {
        ObjectInfo const & objectInfo = mapValue(wrappedObjects_, objectId);
        if (objectInfo.object && !objectInfo.classinfo.toMap().empty())
            return objectInfo.object;
        Object *object = mapValue(registeredObjects_, objectId);
        if (object)
            return object;
    }

    warning("No wrapped object", objectId);
    return nullptr;
}

Value Publisher::toVariant(Value &&value, int targetType) const
{
    (void) targetType;
    if (targetType == Value::Object_) {
        Object *unwrappedObject = unwrapObject(mapValue(value.toMap(), KEY_ID).toString());
        if (unwrappedObject == nullptr)
            warning("Cannot not convert non-object argument to Object*.", value);
        return unwrappedObject;
    }
    return std::move(value);
}

void Publisher::transportRemoved(Transport *transport)
{
    auto it = transportedWrappedObjects_.find(transport);
    // It is not allowed to modify a container while iterating over it. So save
    // objects which should be removed and call objectDestroyed() on them later.
    std::vector <Object*> objectsForDeletion;
    while (it != transportedWrappedObjects_.end() && it->first == transport) {
        if (mapContains(wrappedObjects_, it->second)) {
            std::vector <Transport*> &transports = wrappedObjects_[it->second].transports;
            remove(transports, transport);
            if (transports.empty())
                objectsForDeletion.emplace_back(wrappedObjects_[it->second].object);
        }

        it++;
    }

    transportedWrappedObjects_.erase(transport);

    for (Object *obj : objectsForDeletion)
        objectDestroyed(obj);
}

// NOTE: transport can be a nullptr
//       in such a case, we need to ensure that the property is registered to
//       the target transports of the parentObjectId
Value Publisher::wrapResult(Value &&result, Transport *transport,
                                            const std::string &parentObjectId)
{
    if (Object *object = result.toObject()) {
        std::string id = mapValue(objectIds_, object);

        Value classInfo;
        if (id.empty()) {
            // neither registered, nor wrapped, do so now
            id = channel_->createUuid();
            // store ID before the call to classInfoForObject()
            // in case of self-contained objects it avoids
            // infinite loops
            objectIds_[object] = id;

            classInfo = classInfoForObject(object, transport);

            ObjectInfo oi(object, std::move(classInfo));
            if (transport) {
                oi.transports.emplace_back(transport);
            } else {
                // use the transports from the parent object
                oi.transports = mapValue(wrappedObjects_, parentObjectId).transports;
                // or fallback to all transports if the parent is not wrapped
                if (oi.transports.empty())
                    oi.transports = channel_->transports_;
            }
            wrappedObjects_.insert(std::make_pair(id, std::move(oi)));
            transportedWrappedObjects_.insert(std::make_pair(transport, id));

            initializePropertyUpdates(object, classInfo.toMap());
        } else if (mapContains(wrappedObjects_, id)) {
            assert(object == mapValue(wrappedObjects_, id).object);
            // check if this transport is already assigned to the object
            if (transport) {
                if (!contains(mapValue(wrappedObjects_, id).transports, transport))
                    wrappedObjects_[id].transports.emplace_back(transport);
                if (!mapContains(transportedWrappedObjects_, transport, id))
                    transportedWrappedObjects_.insert(std::make_pair(transport, id));
            }
            classInfo = mapValue(wrappedObjects_, id).classinfo.ref();
        }

        Map objectInfo;
        objectInfo[KEY_Object] = true;
        objectInfo[KEY_ID] = id;
        if (!classInfo.toMap().empty())
            objectInfo[KEY_DATA] = std::move(classInfo);

        return std::move(objectInfo);
    }

    return std::move(result);
}

Array Publisher::wrapList(Array &list, Transport *transport, const std::string &parentObjectId)
{
    Array array;
    for (Value & arg : list) {
        array.emplace_back(wrapResult(std::move(arg), transport, parentObjectId));
    }
    return array;
}

void Publisher::deleteWrappedObject(Object *object) const
{
    if (!mapContains(wrappedObjects_, mapValue(objectIds_, object))) {
        warning("Not deleting non-wrapped object", object);
        return;
    }
    //object->deleteLater();
}

void Publisher::broadcastMessage(const Message &message) const
{
    if (channel_->transports_.empty()) {
        warning("QWebChannel is not connected to any transports, cannot send message: %s", message);
        return;
    }

    for (Transport *transport : channel_->transports_) {
        transport->sendMessage(message);
    }
}

void Publisher::handleMessage(Message &&message, Transport *transport)
{
    if (!mapContains(message, KEY_TYPE)) {
        warning("JSON message object is missing the type property: %s", message);
        return;
    }

    const MessageType type = toType(mapValue(message, KEY_TYPE));
    if (type == TypeIdle) {
        setClientIsIdle(true);
    } else if (type == TypeInit) {
        if (!mapContains(message, KEY_ID)) {
            warning("JSON message object is missing the id property: %s", message);
            return;
        }
        transport->sendMessage(createResponse(std::move(mapValue(message, KEY_ID)), initializeClient(transport)));
    } else if (type == TypeDebug) {
        warning("DEBUG: ", mapValue(message, KEY_DATA));
    } else if (mapContains(message, KEY_OBJECT)) {
        const std::string &objectName = mapValue(message, KEY_OBJECT).toString();
        Object *object = mapValue(registeredObjects_, objectName);
        if (!object)
            object = mapValue(wrappedObjects_, objectName).object;

        if (!object) {
            warning("Unknown object encountered", objectName);
            return;
        }

        if (type == TypeInvokeMethod) {
            if (!mapContains(message, KEY_ID)) {
                warning("JSON message object is missing the id property: %s", message);
                return;
            }

            //QPointer<Publisher> publisherExists(this);
            //QPointer<Transport> transportExists(transport);
            Value args;
            Array args2;
            invokeMethod(object,
                         static_cast<size_t>(mapValue(message, KEY_METHOD).toInt(-1)),
                         std::move(mapValue(message, KEY_ARGS).toArray(args2)), [this, transport, &message] (Value && result) {
                //if (!publisherExists || !transportExists)
                //    return;
                transport->sendMessage(createResponse(mapValue(message, KEY_ID).toString(),
                                                      wrapResult(std::move(result), transport)));
            });
        } else if (type == TypeConnectToSignal) {
            signalHandler_.connectTo(object, static_cast<size_t>(mapValue(message, KEY_SIGNAL).toInt(-1)));
        } else if (type == TypeDisconnectFromSignal) {
            signalHandler_.disconnectFrom(object, static_cast<size_t>(mapValue(message, KEY_SIGNAL).toInt(-1)));
        } else if (type == TypeSetProperty) {
            setProperty(object, static_cast<size_t>(mapValue(message, KEY_PROPERTY).toInt(-1)),
                        std::move(mapValue(message, KEY_VALUE)));
        }
    }
}

void Publisher::propertyChanged(const Object *object, size_t propertyIndex)
{
    if (!channel_ || channel_->transports_.empty()) {
        return;
    }
    pendingPropertyUpdates2_[object].insert(propertyIndex);
    if (clientIsIdle_ && !blockUpdates_) {
        channel_->startTimer(PROPERTY_UPDATE_INTERVAL);
    }
}

void Publisher::setBlockUpdates(bool block)
{
    if (blockUpdates_ == block) {
        return;
    }
    blockUpdates_ = block;

    if (!blockUpdates_) {
        sendPendingPropertyUpdates();
    } else {
        channel_->stopTimer();
    }

    //blockUpdatesChanged(block);
}

void Publisher::timerEvent()
{
    sendPendingPropertyUpdates();
}

