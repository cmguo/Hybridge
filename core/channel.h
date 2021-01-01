#ifndef CHANNEL_H
#define CHANNEL_H

#include "Hybridge_global.h"
#include "object.h"
#include "message.h"

#include <map>
#include <string>
#include <vector>
#include <functional>

class Transport;
class Publisher;
class TestBridge;

class HYBRIDGE_EXPORT Channel
{
public:
    explicit Channel();

    Channel(Channel const & o) = delete;

    Channel& operator=(Channel const & o) = delete;

    virtual ~Channel();

public:
    void registerObjects(const std::unordered_map<std::string , Object*> &objects);

    std::unordered_map<std::string , Object*> registeredObjects() const;

    void registerObject(const std::string  &id, Object *object);

    void deregisterObject(Object *object);

    bool blockUpdates() const;

    void setBlockUpdates(bool block);

//Q_SIGNALS:
//    void blockUpdatesChanged(bool block);

public:
    typedef std::function<void(Value &&)> response_t;

    void connectTo(Transport *transport, response_t receive);

    void disconnectFrom(Transport *transport);

protected:
    virtual MetaObject * metaObject(Object const * object) const = 0;

    virtual std::string createUuid() const = 0;

    virtual MetaObject::Connection connect(Object const * object, size_t signalIndex) = 0;

    virtual bool disconnect(MetaObject::Connection const & c) = 0;

    virtual void startTimer(int msec) = 0;

    virtual void stopTimer() = 0;

protected:
    void messageReceived(Message &&message, Transport *transport);

    void signal(Object const * object, size_t signalIndex, Array && args);

    void propertyChanged(Object const * object, size_t propertyIndex);

    void timerEvent();

private:
    void init();

private:
    friend class Publisher;
    friend class TestBridge;
    friend class Receiver;
    friend class Transport;
    friend class SignalHandler;

    Publisher * publisher_;
    std::vector<Transport*> transports_;
    std::map<Transport*, Receiver*> receivers_;
};

#endif // CHANNEL_H
