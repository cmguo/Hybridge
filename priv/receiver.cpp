#include "receiver.h"
#include "core/channel.h"
#include "collection.h"
#include "debug.h"
#include "core/transport.h"

Receiver::Receiver(Channel * channel, Transport *transport)
    : channel_(channel)
    , transport_(transport)
{
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
    }
}

void Receiver::init(response_t response)
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

void Receiver::invokeMethod(const ProxyObject *object, size_t methodIndex, Array &&args, response_t response)
{
    Message message;
    message[KEY_TYPE] = TypeInvokeMethod;
    message[KEY_ID] = object->id_;
    message[KEY_METHOD] = static_cast<int>(methodIndex);
    message[KEY_ARGS] = std::move(args);
    sendMessage(message, [this, response](Value && data) {
        if (mapValue(data.toMap(), KEY_Object).toBool()) {
            Map emptyMap;
            data = unwrapObject(std::move(data.toMap(emptyMap)));
        }
        response(std::move(data));
    });
}

void Receiver::connectTo(const ProxyObject *object, size_t signalIndex)
{
    Message message;
    message[KEY_TYPE] = TypeConnectToSignal;
    message[KEY_ID] = object->id_;
    message[KEY_SIGNAL] = static_cast<int>(signalIndex);
    transport_->sendMessage(message);
}

void Receiver::disconnectFrom(const ProxyObject *object, size_t signalIndex)
{
    Message message;
    message[KEY_TYPE] = TypeDisconnectFromSignal;
    message[KEY_ID] = object->id_;
    message[KEY_SIGNAL] = static_cast<int>(signalIndex);
    transport_->sendMessage(message);
}

void Receiver::setProperty(ProxyObject *object, size_t propertyIndex, Value &&value)
{
    Message message;
    message[KEY_TYPE] = TypeDisconnectFromSignal;
    message[KEY_ID] = object->id_;
    message[KEY_PROPERTY] = static_cast<int>(propertyIndex);
    message[KEY_VALUE] = std::move(value);
    transport_->sendMessage(message);
}

void Receiver::sendMessage(Message &message, Receiver::response_t response)
{
    std::string id = stringNumber(msgId_++);
    message[KEY_ID] = id;
    responses_[id] = response;
    transport_->sendMessage(message);
}

void Receiver::response(const std::string &id, Value &&result)
{
    response_t resp = responses_[id];
    responses_.erase(id);
    resp(std::move(result));
}

ProxyObject *Receiver::unwrapObject(Map &&data)
{
    std::string id = data[KEY_ID].toString();
    if (id.empty()) {
        warning("");
        return nullptr;
    }
    if (mapContains(objects_, id)) {
        return mapValue(objects_, id);
    }
    ProxyObject * obj = new ProxyObject(this, id, std::move(data));
    objects_[id] = obj;
    connectTo(obj, 0);
    return obj;
}
