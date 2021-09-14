#ifndef PROXYOBJECT_H
#define PROXYOBJECT_H

#include "core/value.h"

#include <functional>

class Transport;
class Receiver;
class MetaObject;
class MetaProperty;
class MetaMethod;
class MetaEnum;

class HYBRIDGE_EXPORT ProxyObject
{
public:
    ProxyObject(Map && classinfo);

    virtual ~ProxyObject() = default;

    DELETE_COPY(ProxyObject)

    std::string const & id() const { return id_; }

    // The class meta of this object, contains properties, methods, signals
    MetaObject * metaObj() const { return metaObj_; }

    // A handle stands for proxy implmentation,
    //   used in signal handler or invoke callback
    virtual void * handle() const { return const_cast<ProxyObject*>(this); }

protected:
    MetaProperty const * property(char const * name) const ;

    MetaMethod const * method(char const * name) const;

    MetaEnum const * enumerator(char const * name) const;

private:
    // Tell where then object comes from,
    //   this is a connection to real object
    Receiver * receiver() const { return receiver_; }

    void init(Receiver * receiver, std::string const & id);

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
