#ifndef RECEIVER_H
#define RECEIVER_H

#include "core/metaobject.h"
#include "core/message.h"
#include "core/proxyobject.h"

class Channel;
class Transport;

class Receiver
{
public:
    Receiver(Channel * channel, Transport *transport);

    ~Receiver();

public:
    /**
     * Handle the @p message and if needed send a response to @p transport.
     */
    void handleMessage(Message &&message);

protected:
    friend class ProxyMetaProperty;
    friend class ProxyMetaMethod;
    friend class ProxyMetaObject;

    typedef MetaMethod::Response Response;

    void init(Response const & response);

    bool invokeMethod(ProxyObject *object, size_t methodIndex, Array &&args, Response const & response);

    bool connectToSignal(MetaObject::Connection const & conn);

    bool disconnectFromSignal(MetaObject::Connection const & conn);

    bool setProperty(ProxyObject *object, size_t propertyIndex, Value &&value);

protected:
    void sendMessage(Message &&message, Response const & response);

    void response(std::string const & id, Value && result);

private:
    Array unwrapList(Array &list);

    Value unwrapResult(Value &&result);

    Object * unwrapObject(Map && data);

    void onObjectDestroyed(Object const * object);

private:
    friend class Channel;

    Channel * channel_;
    Transport * transport_;
    size_t msgId_ = 0;

    std::unordered_map<std::string, ProxyObject*> objects_;
    std::unordered_map<std::string, Response> responses_;
    std::vector<MetaObject::Connection> connections_;
};

#endif // RECEIVER_H
