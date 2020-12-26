#include "message.h"

const std::string KEY_SIGNALS = ("signals");
const std::string KEY_METHODS = ("methods");
const std::string KEY_PROPERTIES = ("properties");
const std::string KEY_ENUMS = ("enums");
const std::string KEY_Object = ("__Object*__");
const std::string KEY_ID = ("id");
const std::string KEY_DATA = ("data");
const std::string KEY_OBJECT = ("object");
const std::string KEY_DESTROYED = ("destroyed");
const std::string KEY_SIGNAL = ("signal");
const std::string KEY_TYPE = ("type");
const std::string KEY_METHOD = ("method");
const std::string KEY_ARGS = ("args");
const std::string KEY_PROPERTY = ("property");
const std::string KEY_VALUE = ("value");

char const * stringNumber(size_t n)
{
    static char str[16];
    _itoa_s(static_cast<int>(n), str, 10);
    return str;
}


MessageType toType(const Value &value)
{
    int i = value.toInt(-1);
    if (i >= TYPES_FIRST_VALUE && i <= TYPES_LAST_VALUE) {
        return static_cast<MessageType>(i);
    } else {
        return TypeInvalid;
    }
}
