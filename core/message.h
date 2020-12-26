#ifndef MESSAGE_H
#define MESSAGE_H

#include "value.h"

enum MessageType {
    TypeInvalid = 0,

    TYPES_FIRST_VALUE = 1,

    TypeSignal = 1,
    TypePropertyUpdate = 2,
    TypeInit = 3,
    TypeIdle = 4,
    TypeDebug = 5,
    TypeInvokeMethod = 6,
    TypeConnectToSignal = 7,
    TypeDisconnectFromSignal = 8,
    TypeSetProperty = 9,
    TypeResponse = 10,

    TYPES_LAST_VALUE = 10
};

extern const std::string KEY_SIGNALS;
extern const std::string KEY_METHODS;
extern const std::string KEY_PROPERTIES;
extern const std::string KEY_ENUMS;
extern const std::string KEY_Object; // special
extern const std::string KEY_ID;
extern const std::string KEY_DATA;
extern const std::string KEY_OBJECT;
extern const std::string KEY_DESTROYED;
extern const std::string KEY_SIGNAL;
extern const std::string KEY_TYPE;
extern const std::string KEY_METHOD;
extern const std::string KEY_ARGS;
extern const std::string KEY_PROPERTY;
extern const std::string KEY_VALUE;

typedef Map Message;

MessageType toType(const Value &value);

char const * stringNumber(size_t n);

#endif // MESSAGE_H
