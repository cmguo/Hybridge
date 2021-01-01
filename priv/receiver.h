#ifndef RECEIVER_H
#define RECEIVER_H

#include "core/object.h"
#include "core/message.h"

#include "proxyobject.h"

class Channel;
class Transport;

class Receiver
{
public:
    typedef ProxyObject::response_t response_t;

    Receiver(Channel * channel, Transport *transport);

public:
    /**
     * Handle the @p message and if needed send a response to @p transport.
     */
    void handleMessage(Message &&message);

protected:
    friend class ProxyMetaProperty;
    friend class ProxyMetaMethod;

    void init(response_t response);

    void invokeMethod(ProxyObject const *object, size_t methodIndex, Array &&args, response_t response);

    void connectTo(ProxyObject const *object, size_t signalIndex);

    void disconnectFrom(ProxyObject const *object, size_t signalIndex);

    void setProperty(ProxyObject *object, size_t propertyIndex, Value &&value);

protected:
    void sendMessage(Message &message, response_t response);

    void response(std::string const & id, Value && result);

    ProxyObject * unwrapObject(Map && data);

private:
    friend class Channel;

    Channel * channel_;
    Transport * transport_;
    size_t msgId_ = 0;

    std::unordered_map<std::string, ProxyObject*> objects_;
    std::unordered_map<std::string, response_t> responses_;
};

#endif // RECEIVER_H
