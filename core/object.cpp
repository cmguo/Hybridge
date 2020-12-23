#include "object.h"
#include "message.h"
#include "receiver.h"

#include <algorithm>

class ProxyMetaProperty;
class ProxyMetaMethod;
class ProxyMetaEnum;

class ProxyMetaObject : public MetaObject
{
public:
    ProxyMetaObject(Map &&classinfo);

    // MetaObject interface
public:
    virtual const char *className() const override;
    virtual int propertyCount() const override;
    virtual const MetaProperty &property(int index) const override;
    virtual int methodCount() const override;
    virtual const MetaMethod &method(int index) const override;
    virtual int enumeratorCount() const override;
    virtual const MetaEnum &enumerator(int index) const override;

private:
    std::vector<ProxyMetaMethod> methods_;
    std::vector<ProxyMetaProperty> properties_;
    std::vector<ProxyMetaEnum> enums_;
};

ProxyObject::ProxyObject(Receiver * receiver, Transport *transport, std::string const & id, Map &&classinfo)
    : receiver_(receiver)
    , transport_(transport)
    , id_(id)
    , metaObj_(new ProxyMetaObject(std::move(classinfo)))
{
}

void ProxyObject::invokeMethod(const int methodIndex, Array &&args, ProxyObject::response_t response)
{
    receiver_->invokeMethod(this, methodIndex, std::move(args), response);
}

const char *ProxyMetaObject::className() const
{
    return "ProxyObject";
}

int ProxyMetaObject::propertyCount() const
{
    return static_cast<int>(properties_.size());
}

class ProxyMetaProperty : public MetaProperty
{
public:
    ProxyMetaProperty(Array const & property, MetaMethod const & signal) : property_(property), signal_(signal) {}

private:
    // [0] index
    // [1] name
    // [2] signalInfo
    // [3] value
    Array const & property_;
    MetaMethod const & signal_;

    // MetaProperty interface
public:
    virtual const char *name() const override { return property_.at(1).toString().c_str(); }
    virtual bool isValid() const override { return true; }
    virtual int type() const override { return -1; }
    virtual bool isConstant() const override { return false; }
    virtual bool hasNotifySignal() const override { return signal_.isValid(); }
    virtual int notifySignalIndex() const override { return signal_.methodIndex(); }
    virtual const MetaMethod &notifySignal() const override { return signal_; }
    virtual Value read(const Object *) const override { return Value(); }
    virtual bool write(Object *, const Value &) const override { return false; }
};

const MetaProperty &ProxyMetaObject::property(int index) const
{
    return properties_[static_cast<size_t>(index)];
}

int ProxyMetaObject::methodCount() const
{
    return static_cast<int>(methods_.size());
}

class ProxyMetaMethod : public MetaMethod
{
public:
    ProxyMetaMethod(Array const & method, bool isSignal) : method_(method), isSignal_(isSignal) {}

private:
    // [0] name
    // [1] index
    Array const & method_;
    bool isSignal_;

    // MetaMethod interface
public:
    virtual const char *name() const override { return method_.at(0).toString().c_str(); }
    virtual bool isValid() const override { return true; }
    virtual bool isSignal() const override { return isSignal_; }
    virtual bool isPublic() const override { return true; }
    virtual int methodIndex() const override { return method_.at(1).toInt(); }
    virtual const char *methodSignature() const override { return nullptr; }
    virtual int parameterCount() const override { return -1; }
    virtual int parameterType(int) const override { return -1; }
    virtual const char *parameterName(int) const override { return nullptr; }
    virtual Value invoke(Object *, const Array &) const override { return Value(); }
};

const MetaMethod &ProxyMetaObject::method(int index) const
{
    static EmptyMetaMethod emptyMethod;
    for (auto & m : methods_)
        if (m.methodIndex() == index)
            return m;
    return emptyMethod;
}

int ProxyMetaObject::enumeratorCount() const
{
    return static_cast<int>(enums_.size());
}

class ProxyMetaEnum : public MetaEnum
{
public:
    ProxyMetaEnum(std::string const & name, Map const & menum) : name_(name), menum_(menum) {}

private:
    std::string const & name_;
    Map const & menum_;

    // MetaEnum interface
public:
    virtual const char *name() const override { return name_.c_str(); }
    virtual int keyCount() const override { return static_cast<int>(menum_.size()); }
    virtual const char *key(int index) const override;
    virtual int value(int index) const override;
};

const MetaEnum &ProxyMetaObject::enumerator(int index) const
{
    return enums_[static_cast<size_t>(index)];
}

ProxyMetaObject::ProxyMetaObject(Map &&classinfo)
{
    Array emptyArray;
    Array & signals = classinfo[KEY_SIGNALS].toArray(emptyArray);
    for (Value & s : signals)
        methods_.emplace_back(ProxyMetaMethod(s.toArray(emptyArray), true));
    Array & methods = classinfo[KEY_METHODS].toArray(emptyArray);
    for (Value & m : methods)
        methods_.emplace_back(ProxyMetaMethod(m.toArray(emptyArray), false));
    Map emptyMap;
    Map & enums = classinfo[KEY_ENUMS].toMap(emptyMap);
    for (auto & e : enums)
        enums_.emplace_back(ProxyMetaEnum(e.first, e.second.toMap(emptyMap)));
    Array & props = classinfo[KEY_PROPERTIES].toArray(emptyArray);
    for (Value & v : props) {
        Array & propertyInfo = v.toArray(emptyArray);
        Array & signalInfo = propertyInfo.at(2).toArray(emptyArray);
        static EmptyMetaMethod emptyMethod;
        properties_.emplace_back(ProxyMetaProperty(propertyInfo,
                signalInfo.empty() ? emptyMethod : method(signalInfo.at(1).toInt())));
    }
}

const char *ProxyMetaEnum::key(int index) const
{
    auto it = menum_.begin();
    std::advance(it, index);
    return it->first.c_str();
}

int ProxyMetaEnum::value(int index) const
{
    auto it = menum_.begin();
    std::advance(it, index);
    return it->second.toInt();
}
