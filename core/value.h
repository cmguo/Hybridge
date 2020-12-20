#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <map>
#include <unordered_map>
#include <vector>

#include <assert.h>

class Value;

typedef void Object;

class ValueRef
{
public:
    ValueRef() : v_(nullptr), d_(nullptr) {}

    template<typename T>
    ValueRef(T && t) : v_(new T(std::move(t))), d_(&deleter<T>) {}

    template<typename T, typename std::enable_if_t<std::is_copy_constructible<T>::value> * = nullptr>
    ValueRef(T const & t) : v_(new T(t)), d_(&deleter<T>) {}

    ValueRef(ValueRef const & o) = delete;

    ValueRef(ValueRef && o) : v_(nullptr), d_(nullptr) { swap(o); }

    ValueRef& operator=(ValueRef const & o) = delete;

    ValueRef& operator=(ValueRef && o) { swap(o); return *this; }

    ~ValueRef() { if (v_) d_(v_); }

    template<typename T>
    ValueRef ref() const { assert(is<T>()); ValueRef r; r.v_ = v_; r.d_ = &reference<T>; return r; }

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

class Value
{
public:
    Value() {}

    Value(Value const & o) = delete;

    Value(Value && o) : r_(std::move(o.r_)) {}

    Value& operator=(Value const & o) = delete;

    Value& operator=(Value && o) { r_ = std::move(o.r_); return *this; }

    template<typename T>
    Value ref() const { Value v; v.r_ = r_.ref<T>(); return v; }

public:
    Value(bool b) : r_(std::move(b)) {}
    bool toBool(bool dft = false) const { return r_.as(dft); }

    Value(int n) : Value(static_cast<long int>(n)) {}
    Value(long int n) : r_(std::move(n)) {}
    long int toInt(long int dft = 0) const { return r_.as(dft); }

    Value(double d) : r_(std::move(d)) {}
    double toDouble(double dft = 0) const { return r_.as(dft); }

    Value(std::string const & s) : r_(s) {}
    std::string const & toString(std::string const &dft = std::string()) const { return r_.as(dft); }

    Value(std::string && s) : r_(std::move(s)) {}
    std::string & toString(std::string & dft) const { return r_.as(dft); }

//    MsgValue(MsgMap const & m) : r_(m) {}
    Map const & toMap(Map const & dft = dftMap) const { return r_.as(dft); }

    Value(Map && m) : r_(std::move(m)) {}
    Map & toMap(Map & dft) const { return r_.as(dft); }

//    MsgValue(MsgArray const & a) : r_(a) {}
    Array const & toArray(Array const & dft = dftArray) const { return r_.as(dft); }

    Value(Array && a) : r_(std::move(a)) {}
    Array & toArray(Array & dft) const { return r_.as(dft); }

    Value(Object * o) : r_(std::move(o)) {}
    Object * toObject() const { return r_.as(static_cast<Object *>(nullptr)); }

private:
    static Map const dftMap;
    static Array const dftArray;
    ValueRef r_;
};

namespace std {

template <>
map<std::string, Value>::map(map const & ) = delete;

template <>
map<std::string, Value> & map<std::string, Value>::operator=(map const & ) = delete;

}

#endif // VALUE_H
