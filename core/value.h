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

typedef std::map<std::string, Value> Map;
typedef std::vector<Value> Array;

class HYBRIDGE_EXPORT Value
{
public:
    enum Type
    {
        None,
        Bool,
        Int,
        Long,
        Float,
        Double,
        String,
        Array_,
        Map_,
        Object_,
    };

public:
    Value() : v_(nullptr), t_(None), r_(CRef) {}

    Value(Value && o) : Value() { swap(*this, o); }

    Value(Type t, void * v) : v_(v), t_(t), r_(Ref) {}

    Value(Type t, void const * v) : v_(const_cast<void*>(v)), t_(t), r_(CRef) {}

    Value& operator=(Value && o) { swap(*this, o); return *this; }

    ~Value() { if (r_ == Val) destroys[t_](v_); }

    DELETE_COPY(Value)

    Value ref() const { Value v; v.v_ = v_; v.t_ = t_; v.r_ = r_ == CRef ? CRef : Ref; return v; }

    static Value adopt(Type t, void * v) { Value vl(t, v); vl.r_ = Val; return vl; }

public:
    Value(bool b) : Value(std::move(b), 0) {}
    bool isBool() const { return t_ == Bool; }
    bool toBool(bool dft = false) const { return value(dft); }

    Value(int n) : Value(std::move(n), 0) {}
    bool isInt() const { return t_ == Int; }
    int toInt(int dft = 0) const { return value(dft); }

    Value(long long n) : Value(std::move(n), 0) {}
    bool isLong() const { return t_ == Long; }
    long long toLong(long long dft = 0) const { return value(dft); }

    Value(float d) : Value(std::move(d), 0) {}
    bool isFloat() const { return t_ == Float; }
    float toFloat(float dft = 0) const { return value(dft); }

    Value(double d) : Value(std::move(d), 0) {}
    bool isDouble() const { return t_ == Double; }
    double toDouble(double dft = 0) const { return value(dft); }

    Value(std::string && s) : Value(std::move(s), 0) {}
    Value(char const * s) : Value(std::string(s), 0) {}
    Value(std::string const & s) : Value(std::string(s), 0) {}
    bool isString() const { return t_ == String; }
    std::string & toString(std::string & dft) const { return unref(dft); }
    std::string const & toString(std::string const &dft = std::string()) const { return unref(dft); }

    Value(Array && a) : Value(std::move(a), 0) {}
    Value(Array & m) : Value(m, 0) {}
    Value(Array const & m) : Value(m, 0) {}
    bool isArray() const { return t_ == Array_; }
    Array & toArray(Array & dft) const { return unref(dft); }
    Array const & toArray(Array const & dft = dftArray) const { return unref(dft); }

    Value(Map && m) : Value(std::move(m), 0) {}
    Value(Map & m) : Value(m, 0) {}
    Value(Map const & m) : Value(m, 0) {}
    bool isMap() const { return t_ == Map_; }
    Map & toMap(Map & dft) const { return unref(dft); }
    Map const & toMap(Map const & dft = dftMap) const { return unref(dft); }

    Value(Object * o) : Value(std::move(o), 0) {}
    bool isObject() const { return t_ == Object_; }
    Object * toObject() const { return unref(static_cast<Object *>(nullptr)); }

    static Map const dftMap;
    static Array const dftArray;

    static Value fromJson(std::string const & json);
    static std::string toJson(Value const & value);

    Type type() const
    {
        return t_;
    }

    void * value() { return v_; }

    void const * value() const { return v_; }

private:
    enum Refr
    {
        Val, // Own a value instanse, delete on destory
        Ref, // Own a mutable outer instanse
        CRef // Own a const outer instanse
    };

    template<typename T>
    struct TypeOf
    {
        static constexpr int value = -1;
    };

    template<> struct TypeOf<bool> { static constexpr Type value = Bool; };
    template<> struct TypeOf<int> { static constexpr Type value = Int; };
    template<> struct TypeOf<long long> { static constexpr Type value = Long; };
    template<> struct TypeOf<float> { static constexpr Type value = Float; };
    template<> struct TypeOf<double> { static constexpr Type value = Double; };
    template<> struct TypeOf<std::string> { static constexpr Type value = String; };
    template<> struct TypeOf<Array> { static constexpr Type value = Array_; };
    template<> struct TypeOf<Map> { static constexpr Type value = Map_; };
    template<> struct TypeOf<Object*> { static constexpr Type value = Object_; };

    template<typename T>
    static void destroy(void * v) { delete reinterpret_cast<T*>(v); }

    template<typename T>
    static void * null_conv(void *) { return new T(); }

    template<typename F, typename T>
    static void * conv(void * v) { return new T(static_cast<T>(*reinterpret_cast<F*>(v))); }

    template<typename F>
    static void * conv_str(void * v) { return new std::string(std::to_string(*reinterpret_cast<F*>(v))); }

    static constexpr void (*destroys[])(void *) = { nullptr,
            destroy<bool>, destroy<int>, destroy<long long>,
            destroy<float>, destroy<double>, destroy<std::string>,
            destroy<Array>, destroy<Map>, destroy<Object*>,
    };

#define CONVS(c) c<bool>, c<int>, c<long long>, c<float>, c<double>, \
    c<std::string>, c<Array>, c<Map>, c<Object*>
#define CONVS2(c, f) c<f, bool>, c<f, int>, c<f, long long>, c<f, float>, c<f, double>, conv_str<f>

    static constexpr void * (*convertors[][10])(void *) = {
        { nullptr, CONVS(null_conv) },
        { nullptr, CONVS2(conv, bool) },
        { nullptr, CONVS2(conv, int) },
        { nullptr, CONVS2(conv, long long) },
        { nullptr, CONVS2(conv, float) },
        { nullptr, CONVS2(conv, double) },
        { nullptr },
        { nullptr },
        { nullptr },
        { nullptr },
    };

    template<typename T>
    Value(T && t, int)
        : v_(new T(std::move(t)))
        , t_(TypeOf<T>::value)
        , r_(Val)
    {
    }

    template<typename T>
    Value(T & t, int)
        : v_(&t)
        , t_(TypeOf<T>::value)
        , r_(Ref)
    {
    }

    template<typename T>
    Value(T const & t, int)
        : v_(const_cast<T *>(&t))
        , t_(TypeOf<T>::value)
        , r_(CRef)
    {
    }

    template<typename T>
    T & unref(T & dflt) const
    {
        if (t_ != TypeOf<T>::value)
            return dflt;
        assert(r_ != CRef);
        return *reinterpret_cast<T*>(v_);
    }

    template<typename T>
    T const & unref(T const & dflt) const
    {
        if (t_ != TypeOf<T>::value)
            return dflt;
        return *reinterpret_cast<T*>(v_);
    }

    template<typename T>
    T value(T dflt) const
    {
        if (t_ == TypeOf<T>::value)
            return *reinterpret_cast<T*>(v_);
        auto conv = convertors[t_][TypeOf<T>::value];
        if (conv) {
            T * t = reinterpret_cast<T*>(conv(v_));
            T tt = *t;
            delete t;
            return tt;
        }
        return dflt;
    }

    friend void swap(Value & l, Value & r)
    {
        std::swap(l.v_, r.v_);
        std::swap(l.t_, r.t_);
        std::swap(l.r_, r.r_);
    }

private:
    void * v_;
    Type t_;
    Refr r_; // reference type
};

namespace std {

template <>
map<std::string, Value>::map(map const & ) = delete;

template <>
map<std::string, Value> & map<std::string, Value>::operator=(map const & ) = delete;

ostream & operator <<(ostream &, Value const &);

}

#endif // VALUE_H
