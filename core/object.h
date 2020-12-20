#ifndef OBJECT_H
#define OBJECT_H

#include "value.h"

typedef void Object;

class MetaMethod
{
public:
    virtual ~MetaMethod() {}

    virtual char const * name() const = 0;

    virtual bool isValid() const = 0;

    virtual bool isSignal() const = 0;

    virtual bool isPublic() const = 0;

    virtual int methodIndex() const = 0;

    virtual char const * methodSignature() const = 0;

    virtual int parameterCount() const = 0;

    virtual int parameterType(int index) const = 0;

    virtual char const * parameterName(int index) const = 0;

    virtual Value invoke(Object * object, Array const & args) const = 0;
};

class MetaProperty
{
public:
    virtual ~MetaProperty() {}

    virtual char const * name() const = 0;

    virtual bool isValid() const = 0;

    virtual int type() const = 0;

    virtual bool isConstant() const = 0;

    virtual bool hasNotifySignal() const = 0;

    virtual int notifySignalIndex() const = 0;

    virtual MetaMethod const & notifySignal() const = 0;

    virtual Value read(Object const * object) const = 0;

    virtual bool write(Object const * object, Value const & value) const = 0;
};

class EmptyMetaMethod : public MetaMethod
{
public:
    virtual const char *name() const override { return nullptr; }
    virtual bool isValid() const override { return false; }
    virtual bool isSignal() const override { return false; }
    virtual bool isPublic() const override { return false; }
    virtual int methodIndex() const override { return -1; }
    virtual const char *methodSignature() const override { return nullptr; }
    virtual int parameterCount() const override { return -1; }
    virtual int parameterType(int) const override { return -1; }
    virtual const char *parameterName(int) const override { return nullptr; }
    virtual Value invoke(Object *, const Array &) const override { return Value(); }
};

class MetaEnum
{
public:
    virtual ~MetaEnum() {}

    virtual char const * name() const = 0;

    virtual int keyCount() const = 0;

    virtual char const * key(int index) const = 0;

    virtual int value(int index) const = 0;

};

class MetaObject
{
public:
    virtual ~MetaObject() {}

    virtual char const * className() const = 0;

    virtual int propertyCount() const = 0;

    virtual MetaProperty const & property(int index) const = 0;

    virtual int methodCount() const = 0;

    virtual MetaMethod const & method(int index) const = 0;

    virtual int enumeratorCount() const = 0;

    virtual MetaEnum const & enumerator(int index) const = 0;

    class Connection
    {
    public:
        operator bool() const { return object_; }

    protected:
        Object * object_ = nullptr;
        int signalIndex_ = 0;
    };
};

#include <functional>

class Transport;
class Receiver;

class ProxyObject
{
public:
    typedef std::function<void(Value &&)> response_t;

    ProxyObject(Receiver * receiver, Transport * transport, std::string const & id, Map && classinfo);

protected:
    void invokeMethod(const int methodIndex, Array &&args, response_t response);

    void connectTo(const int signalIndex);

    void disconnectFrom(const int signalIndex);

    void setProperty(const int propertyIndex, Value &&value);

private:
    friend class Receiver;

    Receiver * receiver_ = nullptr;
    Transport * transport_ = nullptr;
    std::string id_;
    MetaObject * metaObj_ = nullptr;
};

#endif // OBJECT_H
