#ifndef RECEIVER_H
#define RECEIVER_H

#include "core/object.h"
#include "core/message.h"

class Bridge;
class Transport;

class Receiver
{
public:
    typedef ProxyObject::response_t response_t;

    Receiver(Bridge * bridge);

public:
    /**
     * Handle the @p message and if needed send a response to @p transport.
     */
    void handleMessage(Message &&message, Transport *transport);

protected:
    friend class ProxyObject;

    void init(Transport *transport);

    void invokeMethod(ProxyObject const *object, const int methodIndex, Array &&args, response_t response);

    void connectTo(ProxyObject const *object, const int signalIndex);

    void disconnectFrom(ProxyObject const *object, const int signalIndex);

    void setProperty(ProxyObject *object, const int propertyIndex, Value &&value);

protected:
    void sendMessage(Message &message, Transport *transport, response_t response);

    void response(Transport *transport, std::string const & id, Value && result);

    ProxyObject * unwrapObject(Transport *transport, Map && data);

private:
    friend class Bridge;
    friend class TestBridge;

    Bridge * bridge_;
    size_t msgId_ = 0;

    typedef std::unordered_map<std::string, ProxyObject*> TransportedObjectsMap;
    std::unordered_map<Transport*, TransportedObjectsMap> transportedObjects_;
    std::unordered_map<std::string, Transport*> responsesTransport_;
    std::unordered_map<std::string, response_t> responses_;
};

#endif // RECEIVER_H
