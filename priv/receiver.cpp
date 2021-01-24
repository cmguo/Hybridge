#include "receiver.h"
#include "core/channel.h"
#include "collection.h"
#include "debug.h"
#include "core/transport.h"

Receiver::Receiver(Channel * channel, Transport *transport)
    : channel_(channel)
    , transport_(transport)
{
    transport_->setReceiver(this);
}

Receiver::~Receiver()
{
    transport_->setReceiver(nullptr);
}

void Receiver::handleMessage(Message &&message)
{
    if (!mapContains(message, KEY_TYPE)) {
        warning("JSON message object is missing the type property: %s", message);
        return;
    }

    const MessageType type = toType(mapValue(message, KEY_TYPE));
    if (type == TypeResponse) {
        if (!mapContains(message, KEY_ID)) {
            warning("JSON message object is missing the id property: %s", message);
            return;
        }
        response(mapValue(message, KEY_ID).toString(), std::move(mapValue(message, KEY_DATA)));
    } else if (mapContains(message, KEY_OBJECT)) {
        const std::string &objectName = mapValue(message, KEY_OBJECT).toString();
        ProxyObject *object = mapValue(objects_, objectName);
        if (!object) {
            warning("Unknown object encountered", objectName);
            return;
        }
        if (type == TypePropertyUpdate) {

        } else if (type == TypeSignal) {
            size_t signalIndex = static_cast<size_t>(mapValue(message, KEY_SIGNAL).toInt());
            Array empty;
            Array & args = mapValue(message, KEY_ARGS).toArray(empty);
            MetaObject::Signal signal(object, signalIndex);
            for (auto & conn : connections_) {
                if (signal == conn) {
                    conn.signal(std::move(args));
                }
            }
        }
    }
}

void Receiver::init(Response const & response)
{
    Message message;
    message[KEY_TYPE] = TypeInit;
    sendMessage(message, [this, response](Value && data) {
        Map emptyMap;
        Map & objectInfos = data.toMap(emptyMap);
        for (auto & o : objectInfos) {
            Map & objectInfo = o.second.toMap(emptyMap);
            objectInfo[KEY_ID] = o.first;
            o.second = unwrapObject(std::move(objectInfo));
        }
        response(std::move(data));
    });
}

bool Receiver::invokeMethod(ProxyObject *object, size_t methodIndex, Array &&args, Response const & response)
{
    Message message;
    message[KEY_TYPE] = TypeInvokeMethod;
    message[KEY_OBJECT] = object->id();
    message[KEY_METHOD] = static_cast<int>(methodIndex);
    message[KEY_ARGS] = std::move(args);
    sendMessage(message, [this, response](Value && data) {
        if (mapValue(data.toMap(), KEY_Object).toBool()) {
            Map emptyMap;
            data = unwrapObject(std::move(data.toMap(emptyMap)));
        }
        response(std::move(data));
    });
    return true;
}

bool Receiver::connectToSignal(const MetaObject::Connection &conn)
{
    Message message;
    message[KEY_TYPE] = TypeConnectToSignal;
    message[KEY_ID] = static_cast<ProxyObject const *>(conn.object())->id();
    message[KEY_SIGNAL] = static_cast<int>(conn.signalIndex());
    transport_->sendMessage(message);
    return true;
}

bool Receiver::disconnectFromSignal(const MetaObject::Connection &conn)
{
    Message message;
    message[KEY_TYPE] = TypeDisconnectFromSignal;
    message[KEY_ID] = static_cast<ProxyObject const *>(conn.object())->id();
    message[KEY_SIGNAL] = static_cast<int>(conn.signalIndex());
    transport_->sendMessage(message);
    return true;
}

bool Receiver::setProperty(ProxyObject *object, size_t propertyIndex, Value &&value)
{
    Message message;
    message[KEY_TYPE] = TypeDisconnectFromSignal;
    message[KEY_ID] = object->id();
    message[KEY_PROPERTY] = static_cast<int>(propertyIndex);
    message[KEY_VALUE] = std::move(value);
    transport_->sendMessage(message);
    return true;
}

void Receiver::sendMessage(Message &message, Response const & response)
{
    std::string id = stringNumber(msgId_++);
    message[KEY_ID] = id;
    responses_[id] = response;
    transport_->sendMessage(message);
}

void Receiver::response(const std::string &id, Value &&result)
{
    Response resp = responses_[id];
    responses_.erase(id);
    resp(std::move(result));
}

Array Receiver::unwrapList(Array &list)
{
    Array array;
    for (Value & arg : list) {
        array.emplace_back(unwrapResult(std::move(arg)));
    }
    return array;
}

Value Receiver::unwrapResult(Value &&result)
{
    Map empty;
    Map & objectInfo = result.toMap(empty);
    if (mapContains(objectInfo, KEY_Object)) {
        return unwrapObject(std::move(objectInfo));
    }
    return std::move(result);
}

Object *Receiver::unwrapObject(Map &&data)
{
    std::string id = data[KEY_ID].toString();
    if (id.empty()) {
        warning("");
        return nullptr;
    }
    if (mapContains(objects_, id)) {
        return mapValue(objects_, id);
    }
    ProxyObject * obj = channel_->createProxyObject(std::move(data));
    obj->init(this, id);
    objects_[id] = obj;
    connectToSignal(MetaObject::Connection(obj, 0));
    return obj->handle();
}
