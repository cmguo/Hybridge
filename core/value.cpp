#include "value.h"

#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <iostream>

Map const Value::dftMap;
Array const Value::dftArray;

static Map emptyMap;
static Array emptyArray;

using namespace rapidjson;

struct MyHandler
{

    enum State {
        None,
        InObject,
        InArray,
        InValue, // waitting for object value
        End,
    };

    struct Layer
    {
        State s = None;
		std::string key;
        Value v;
    };

    std::vector<Layer> stack;

    MyHandler()
    {
        stack.push_back(Layer());
    }

    bool Value_(Value && v) {
        Layer & l = stack.back();
        if (l.s == InArray) {
            l.v.toArray(emptyArray).emplace_back(std::move(v));
        } else if (l.s == InValue) {
            l.v.toMap(emptyMap).insert(std::make_pair(l.key, std::move(v)));
            l.s = InObject;
        } else {
            return false;
        }
        return true;
    }

    bool Null() { return Value_(Value()); }
    bool Bool(bool b) { return Value_(b); }
    bool Int(int i) { return Value_(i); }
    bool Uint(unsigned u) { return Value_(static_cast<int>(u)); }
    bool Int64(int64_t i) { return Value_(i); }
    bool Uint64(uint64_t u) { return Value_(static_cast<long long>(u)); }
    bool Double(double d) { return Value_(d); }
    bool RawNumber(const char* str, SizeType length, bool /*copy*/) { return Value_(std::string(str, length)); }
    bool String(const char* str, SizeType length, bool /*copy*/) { return Value_(std::string(str, length)); }
    bool StartObject() {
        Layer & l = stack.back();
        if (l.s == None) {
            l.s = InObject;
            l.v = Map();
        } else if (l.s == InArray || l.s == InValue) {
            Layer l2 = { InObject, "", Map() };
            stack.emplace_back(std::move(l2));
        } else {
            return false;
        }
        return true;
    }
    bool Key(const char* str, SizeType length, bool /*copy*/) {
        Layer & l = stack.back();
        if (l.s == InObject) {
            l.key = std::string(str, length);
            l.s = InValue;
            return true;
        }
        return false;
    }
    bool EndObject(SizeType /*memberCount*/) {
        Layer & l = stack.back();
        if (l.s == InObject) {
            if (stack.size() == 1) {
                l.s = End;
                return true;
            } else {
                Value v = std::move(l.v);
                stack.pop_back();
                return Value_(std::move(v));
            }
        }
        return false;
    }
    bool StartArray() {
        Layer & l = stack.back();
        if (l.s == None) {
            l.s = InArray;
            l.v = Array();
        } else if (l.s == InArray || l.s == InValue) {
            Layer l2 = { InArray, "", Array() };
            stack.emplace_back(std::move(l2));
        } else {
            return false;
        }
        return true;
    }
    bool EndArray(SizeType /*elementCount*/) {
        Layer & l = stack.back();
        if (l.s == InArray) {
            if (stack.size() == 1) {
                l.s = End;
                return true;
            } else {
                Value v = std::move(l.v);
                stack.pop_back();
                return Value_(std::move(v));
            }
        }
        return false;
    }
};

static void writeValue(Writer<StringBuffer> & writer, Value const & v)
{
    if (v.isInt())
        writer.Int(v.toInt());
    else if (v.isLong())
        writer.Int64(v.toLong());
    else if (v.isBool())
        writer.Bool(v.toBool());
    else if (v.isFloat())
        writer.Double(static_cast<double>(v.toFloat()));
    else if (v.isDouble())
        writer.Double(v.toDouble());
    else if (v.isString())
        writer.String(v.toString().c_str());
    else if (v.isArray()) {
        Array const & a = v.toArray();
        writer.StartArray();
        for (Value const & v2 : a) {
            writeValue(writer, v2);
        }
        writer.EndArray(static_cast<unsigned>(a.size()));
    } else if (v.isMap()) {
        Map const & n = v.toMap();
        writer.StartObject();
        for (auto const & v2 : n) {
            writer.Key(v2.first.c_str());
            writeValue(writer, v2.second);
        }
        writer.EndObject(static_cast<unsigned>(n.size()));
    } else {
        writer.Null();
    }
}

Value Value::fromJson(const std::string &json)
{
    MyHandler handler;
    Reader reader;
    StringStream ss(json.c_str());
    if (reader.Parse(ss, handler))
        return std::move(handler.stack.front().v);
    else
        return Value();
}

std::string Value::toJson(const Value &value)
{
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    writeValue(writer, value);
    return sb.GetString();
}


std::ostream &std::operator <<(std::ostream & os, const Value & v)
{
    os << Value::toJson(v);
    return os;
}
