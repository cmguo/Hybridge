#include "receiver.h"
#include "bridge.h"
#include "collection.h"
#include "debug.h"
#include "transport.h"

Receiver::Receiver(Bridge * bridge)
    : bridge_(bridge)
{
}

void Receiver::handleMessage(Message &&message, Transport *transport)
{
    if (!contains(bridge_->transports_, transport)) {
        warning("Refusing to handle message of unknown transport:", transport);
        return;
    }

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
        response(transport, mapValue(message, KEY_ID).toString(), std::move(mapValue(message, KEY_DATA)));
    }
}

void Receiver::init(Transport *transport)
{
    Message message;
    message[KEY_TYPE] = TypeInit;
    sendMessage(message, transport, [this, transport](Value && data) {
        Map emptyMap;
        Map & objectInfos = data.toMap(emptyMap);
        for (auto & o : objectInfos) {
            Map & objectInfo = o.second.toMap(emptyMap);
            objectInfo[KEY_ID] = o.first;
            unwrapObject(transport, std::move(objectInfo));
        }
    });
}

void Receiver::invokeMethod(const ProxyObject *object, const int methodIndex, Array &&args, Receiver::response_t response)
{
    Message message;
    message[KEY_TYPE] = TypeInvokeMethod;
    message[KEY_ID] = object->id_;
    message[KEY_METHOD] = methodIndex;
    message[KEY_ARGS] = std::move(args);
    sendMessage(message, object->transport_, response);
}

void Receiver::connectTo(const ProxyObject *object, const int signalIndex)
{
    Message message;
    message[KEY_TYPE] = TypeConnectToSignal;
    message[KEY_ID] = object->id_;
    message[KEY_SIGNAL] = signalIndex;
    object->transport_->sendMessage(message);
}

void Receiver::disconnectFrom(const ProxyObject *object, const int signalIndex)
{
    Message message;
    message[KEY_TYPE] = TypeDisconnectFromSignal;
    message[KEY_ID] = object->id_;
    message[KEY_SIGNAL] = signalIndex;
    object->transport_->sendMessage(message);
}

void Receiver::setProperty(ProxyObject *object, const int propertyIndex, Value &&value)
{
    Message message;
    message[KEY_TYPE] = TypeDisconnectFromSignal;
    message[KEY_ID] = object->id_;
    message[KEY_PROPERTY] = propertyIndex;
    message[KEY_VALUE] = std::move(value);
    object->transport_->sendMessage(message);
}

void Receiver::sendMessage(Message &message, Transport *transport, Receiver::response_t response)
{
    std::string id = stringNumber(msgId_++);
    message[KEY_ID] = id;
    responsesTransport_[id] = transport;
    responses_[id] = response;
    transport->sendMessage(message);
}

void Receiver::response(Transport *transport, const std::string &id, Value &&result)
{
    assert(responsesTransport_[id] == transport);
    response_t resp = responses_[id];
    responses_.erase(id);
    responsesTransport_.erase(id);
    resp(std::move(result));
}

ProxyObject *Receiver::unwrapObject(Transport *transport, Map &&data)
{
    std::string id = data[KEY_ID].toString();
    if (id.empty()) {
        warning("");
        return nullptr;
    }
    TransportedObjectsMap & objects = transportedObjects_[transport];
    if (mapContains(objects, id)) {
        return mapValue(objects, id);
    }
    ProxyObject * obj = new ProxyObject(this, transport, id, std::move(data));
    objects[id] = obj;
    connectTo(obj, 0);
    return obj;
}
