#ifndef VALUE_H
#define VALUE_H

#include "Hybridge_global.h"

#pragma warning( disable: 4251)

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <type_traits>

#include <assert.h>

class Value;

typedef void Object;

#define DELETE_COPY(x) \
    x(x const & o) = delete; \
    x & operator=(x const & o) = delete;

class ValueRef
{
public:
    ValueRef() : v_(nullptr), d_(nullptr) {}

    template<typename T>
    ValueRef(T && t) : v_(new T(std::move(t))), d_(&deleter<T>) {}

    template<typename T, typename std::enable_if_t<std::is_copy_constructible<T>::value> * = nullptr>
    ValueRef(T const & t) : v_(new T(t)), d_(&deleter<T>) {}

    ValueRef(ValueRef && o) : v_(nullptr), d_(nullptr) { swap(o); }

    ValueRef& operator=(ValueRef && o) { swap(o); return *this; }

    ~ValueRef() { if (v_) d_(v_); }

    template<typename T>
    ValueRef ref() const { assert(is<T>()); ValueRef r; r.v_ = v_; r.d_ = &reference<T>; return r; }

    template<typename T>
    static ValueRef ref(T & t) { ValueRef r; r.v_ = &t; r.d_ = &reference<T>; return r; }

    DELETE_COPY(ValueRef)

public:
    void swap(ValueRef & o)
    {
        std::swap(v_, o.v_);
        std::swap(d_, o.d_);
    }

    template<typename T>
    T & as() const { assert(is<T>()); return *reinterpret_cast<T*>(v_); }

    template<typename T>
    T & as(T & dft) const { return is<T>() ? *reinterpret_cast<T*>(v_) : dft; }

    template<typename T>
    T const & as(T const & dft) const { return is<T>() ? *reinterpret_cast<T*>(v_) : dft; }

    template<typename T>
    bool is() const { return d_ == &deleter<T> || d_ == &reference<T>; }

private:
    void * v_;
    void (*d_) (void *);

    template<typename T>
    static void deleter(void * t) { delete reinterpret_cast<T*>(t); }

    template<typename T>
    static void reference(void *) { }
};

typedef std::map<std::string, Value> Map;
typedef std::vector<Value> Array;

class HYBRIDGE_EXPORT Value
{
public:
    Value() {}

    Value(Value && o) : v_(std::move(o.v_)) {}

    Value& operator=(Value && o) { v_ = std::move(o.v_); return *this; }

    DELETE_COPY(Value)

    template<typename T>
    Value ref() const { Value v; v.v_ = v_.ref<T>(); return v; }

    template<typename T>
    static Value ref(T & t) { Value v; v.v_ = ValueRef::ref(t); return v; }

public:
    Value(bool b) : v_(std::move(b)) {}
    bool isBool() const { return v_.is<bool>(); }
    bool toBool(bool dft = false) const { return v_.as(dft); }

    Value(int n) : v_(std::move(n)) {}
    bool isInt() const { return v_.is<int>(); }
    int toInt(int dft = 0) const { return v_.as(dft); }

    Value(long long n) : v_(std::move(n)) {}
    bool isLong() const { return v_.is<long long>(); }
    long long toLong(long long dft = 0) const { return v_.as(dft); }

    Value(double d) : v_(std::move(d)) {}
    bool isDouble() const { return v_.is<double>(); }
    double toDouble(double dft = 0) const { return v_.as(dft); }

    Value(std::string const & s) : v_(s) {}
    bool isString() const { return v_.is<std::string>(); }
    std::string const & toString(std::string const &dft = std::string()) const { return v_.as(dft); }

    Value(std::string && s) : v_(std::move(s)) {}
    std::string & toString(std::string & dft) const { return v_.as(dft); }

//    MsgValue(MsgMap const & m) : r_(m) {}
    bool isMap() const { return v_.is<Map>(); }
    Map const & toMap(Map const & dft = dftMap) const { return v_.as(dft); }

    Value(Map && m) : v_(std::move(m)) {}
    Map & toMap(Map & dft) const { return v_.as(dft); }

//    MsgValue(MsgArray const & a) : r_(a) {}
    bool isArray() const { return v_.is<Array>(); }
    Array const & toArray(Array const & dft = dftArray) const { return v_.as(dft); }

    Value(Array && a) : v_(std::move(a)) {}
    Array & toArray(Array & dft) const { return v_.as(dft); }

    Value(Object * o) : v_(std::move(o)) {}
    bool isObject() const { return v_.is<Object*>(); }
    Object * toObject() const { return v_.as(static_cast<Object *>(nullptr)); }

    static Map const dftMap;
    static Array const dftArray;

    static Value fromJson(std::string const & json);
    static std::string toJson(Value const & value);

private:
    ValueRef v_;
};

namespace std {

template <>
map<std::string, Value>::map(map const & ) = delete;

template <>
map<std::string, Value> & map<std::string, Value>::operator=(map const & ) = delete;

}

#endif // VALUE_H
