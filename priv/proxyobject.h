#ifndef PROXYOBJECT_H
#define PROXYOBJECT_H

#include "core/value.h"

#include <functional>

class Transport;
class Receiver;
class MetaObject;

class ProxyObject
{
public:
    typedef std::function<void(Value &&)> response_t;

    ProxyObject(Receiver * receiver, std::string const & id, Map && classinfo);

protected:
    void invokeMethod(const int methodIndex, Array &&args, response_t response);

private:
    friend class Receiver;
    friend class ProxyMetaProperty;
    friend class ProxyMetaMethod;

    Receiver * receiver_ = nullptr;
    std::string id_;
    MetaObject * metaObj_ = nullptr;
};

#endif // PROXYOBJECT_H
