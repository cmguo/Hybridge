#ifndef OBJECT_H
#define OBJECT_H

#include "Hybridge_global.h"
#include "value.h"

typedef void Object;

class HYBRIDGE_EXPORT MetaMethod
{
public:
    virtual ~MetaMethod() = default;

    virtual char const * name() const = 0;

    virtual bool isValid() const = 0;

    virtual bool isSignal() const = 0;

    virtual bool isPublic() const = 0;

    virtual size_t methodIndex() const = 0;

    virtual char const * methodSignature() const = 0;

    virtual size_t parameterCount() const = 0;

    virtual int parameterType(size_t index) const = 0;

    virtual char const * parameterName(size_t index) const = 0;

    virtual Value invoke(Object * object, Array const & args) const = 0;
};

class HYBRIDGE_EXPORT MetaProperty
{
public:
    virtual ~MetaProperty() = default;

    virtual char const * name() const = 0;

    virtual bool isValid() const = 0;

    virtual int type() const = 0;

    virtual bool isConstant() const = 0;

    virtual bool hasNotifySignal() const = 0;

    virtual size_t notifySignalIndex() const = 0;

    virtual MetaMethod const & notifySignal() const = 0;

    virtual Value read(Object const * object) const = 0;

    virtual bool write(Object * object, Value const & value) const = 0;
};

class EmptyMetaMethod : public MetaMethod
{
public:
    virtual const char *name() const override { return nullptr; }
    virtual bool isValid() const override { return false; }
    virtual bool isSignal() const override { return false; }
    virtual bool isPublic() const override { return false; }
    virtual size_t methodIndex() const override { return size_t(-1); }
    virtual const char *methodSignature() const override { return nullptr; }
    virtual size_t parameterCount() const override { return size_t(-1); }
    virtual int parameterType(size_t) const override { return -1; }
    virtual const char *parameterName(size_t) const override { return nullptr; }
    virtual Value invoke(Object *, const Array &) const override { return Value(); }
};

class HYBRIDGE_EXPORT MetaEnum
{
public:
    virtual ~MetaEnum() = default;

    virtual char const * name() const = 0;

    virtual size_t keyCount() const = 0;

    virtual char const * key(size_t index) const = 0;

    virtual int value(size_t index) const = 0;

};

class MetaObject
{
public:
    virtual ~MetaObject() = default;

    virtual char const * className() const = 0;

    virtual size_t propertyCount() const = 0;

    virtual MetaProperty const & property(size_t index) const = 0;

    virtual size_t methodCount() const = 0;

    virtual MetaMethod const & method(size_t index) const = 0;

    virtual size_t enumeratorCount() const = 0;

    virtual MetaEnum const & enumerator(size_t index) const = 0;

    class Connection
    {
    public:
        Connection(Object const * object = nullptr, size_t signalIndex = 0)
            : object_(object)
            , signalIndex_(signalIndex)
        {}
        operator bool() const { return object_; }

        Object const * object() const { return object_; }
        size_t signalIndex() const { return signalIndex_; }

    protected:
        Object const * object_ = nullptr;
        size_t signalIndex_ = 0;
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
