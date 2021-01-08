#include "meta.h"
#include "channel.h"
#include "priv/publisher.h"

void MetaObject::propertyChanged(Channel * channel, const Object *object, size_t propertyIndex)
{
    Array args;
    args.emplace_back(static_cast<int>(propertyIndex));
    channel->publisher_->propertyChanged(object, propertyIndex);
}

MetaObject::Signal::Signal(const Object *object, size_t signalIndex)
    : object_(object)
    , signalIndex_(signalIndex)
{
}

MetaObject::Slot::Slot(void *receiver, MetaObject::Slot::Handler handler)
    : receiver_(receiver)
    , handler_(handler)
{
}

MetaObject::Connection::Connection(const Object *object, size_t signalIndex, void * receiver, Handler handler)
    : Signal(object, signalIndex)
    , Slot(receiver, handler)
{
}

void MetaObject::Connection::signal(Array && args) const
{
    handler_(receiver_, object_, signalIndex_, std::move(args));
}
