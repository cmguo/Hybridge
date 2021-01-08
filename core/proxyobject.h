#ifndef PROXYOBJECT_H
#define PROXYOBJECT_H

#include "core/value.h"

#include <functional>

class Transport;
class Receiver;
class MetaObject;

class HYBRIDGE_EXPORT ProxyObject
{
public:
    ProxyObject();

    virtual ~ProxyObject() = default;

    DELETE_COPY(ProxyObject)

    std::string const & id() const { return id_; }

    MetaObject * metaObj() const { return metaObj_; }

    virtual void * handle() const { return const_cast<ProxyObject*>(this); }

private:
    Receiver * receiver() const { return receiver_; }

    void init(Receiver * receiver, std::string const & id, Map && classinfo);

private:
    friend class Receiver;
    friend class ProxyMetaObject;
    friend class ProxyMetaProperty;
    friend class ProxyMetaMethod;

    Receiver * receiver_ = nullptr;
    std::string id_;
    MetaObject * metaObj_ = nullptr;
};

#endif // PROXYOBJECT_H
