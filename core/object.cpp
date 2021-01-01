#include "object.h"

MetaObject::Connection::Connection(const Object *object, size_t signalIndex)
    : object_(object)
    , signalIndex_(signalIndex)
{}
