#ifndef METAOBJECT_H
#define METAOBJECT_H

#include "Hybridge_global.h"
#include "value.h"

#include <functional>

typedef void Object;

class Channel;

class HYBRIDGE_EXPORT MetaMethod
{
public:
    typedef std::function<void(Value &&)> Response;

    virtual ~MetaMethod() = default;

    virtual char const * name() const = 0;

    virtual bool isValid() const = 0;

    virtual bool isSignal() const = 0;

    virtual bool isPublic() const = 0;

    virtual size_t methodIndex() const = 0;

    virtual char const * methodSignature() const = 0;

    virtual Value::Type returnType() const = 0;

    virtual size_t parameterCount() const = 0;

    virtual Value::Type parameterType(size_t index) const = 0;

    virtual char const * parameterName(size_t index) const = 0;

    virtual bool invoke(Object * object, Array && args, Response const & resp) const = 0;
};

class HYBRIDGE_EXPORT MetaProperty
{
public:
    virtual ~MetaProperty() = default;

    virtual char const * name() const = 0;

    virtual bool isValid() const = 0;

    virtual Value::Type type() const = 0;

    virtual bool isConstant() const = 0;

    virtual size_t propertyIndex() const = 0;

    virtual bool hasNotifySignal() const = 0;

    virtual size_t notifySignalIndex() const = 0;

    virtual MetaMethod const & notifySignal() const = 0;

    virtual Value read(Object const * object) const = 0;

    virtual bool write(Object * object, Value && value) const = 0;
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
    virtual Value::Type returnType() const override { return Value::None; }
    virtual size_t parameterCount() const override { return size_t(-1); }
    virtual Value::Type parameterType(size_t) const override { return Value::None; }
    virtual const char *parameterName(size_t) const override { return nullptr; }
    virtual bool invoke(Object *, Array &&, Response const &) const override { return false; }
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

class HYBRIDGE_EXPORT MetaObject
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

public:
    class HYBRIDGE_EXPORT Signal
    {
    public:
        Signal() {}
        Signal(Object const * object, size_t signalIndex);
        operator bool() const { return object_; }
        friend bool operator==(Signal const & l, Signal const & r)
        {
            return l.object() == r.object() && l.signalIndex() == r.signalIndex();
        }
        Object const * object() const { return object_; }
        size_t signalIndex() const { return signalIndex_; }

    protected:
        Object const * object_ = nullptr;
        size_t signalIndex_ = 0;
    };

    class HYBRIDGE_EXPORT Slot
    {
    public:
        typedef void (*Handler) (void * receiver, const Object *object, size_t signalIdx, Array && args);

        Slot() {}
        Slot(void * receiver, Handler handler);
        friend bool operator==(Slot const & l, Slot const & r)
        {
            return l.handler_ == r.handler_ && l.receiver_ == r.receiver_;
        }
        void * receiver() const { return receiver_; }

    protected:
        void * receiver_ = nullptr;
        Handler handler_ = nullptr;
    };

    class HYBRIDGE_EXPORT Connection : public Signal, public Slot
    {
    public:
        typedef void (*Handler) (void * receiver, const Object *object, size_t signalIdx, Array && args);

        Connection() {}
        Connection(Object const * object, size_t signalIndex,
                   void * receiver = nullptr, Handler receive = nullptr);
        friend bool operator==(Connection const & l, Connection const & r)
        {
            return static_cast<Signal const &>(l) == r
                    && static_cast<Slot const &>(l) == r;
        }
        void signal(Array && args) const;
    };

    virtual bool connect(Connection const & c) const = 0;

    virtual bool disconnect(Connection const & c) const = 0;

protected:
    void propertyChanged(Channel * channel, Object const * object, size_t propertyIndex);
};

#endif // METAOBJECT_H
