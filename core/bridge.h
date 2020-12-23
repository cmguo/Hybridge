#ifndef BRIDGE_H
#define BRIDGE_H

#include "Hybridge_global.h"
#include "object.h"
#include "message.h"

#include <map>
#include <string>
#include <vector>

class Transport;
class Publisher;
class TestBridge;

class HYBRIDGE_EXPORT Bridge
{
public:
    explicit Bridge();
    Bridge(Bridge const & o) = delete;
    Bridge& operator=(Bridge const & o) = delete;
    virtual ~Bridge();

    void registerObjects(const std::unordered_map<std::string , Object*> &objects);
    std::unordered_map<std::string , Object*> registeredObjects() const;
    void registerObject(const std::string  &id, Object *object);
    void deregisterObject(Object *object);

    bool blockUpdates() const;

    void setBlockUpdates(bool block);

//Q_SIGNALS:
//    void blockUpdatesChanged(bool block);

public:
    void connectTo(Transport *transport);
    void disconnectFrom(Transport *transport);

protected:
    virtual MetaObject * metaObject(Object const * object) const = 0;

    virtual std::string createUuid() const = 0;

    virtual MetaObject::Connection connect(Object const * object, int signalIndex) = 0;

    virtual bool disconnect(MetaObject::Connection const & c) = 0;

    virtual void startTimer(int msec) = 0;

    virtual void stopTimer() = 0;

    void messageReceived(Message &&message, Transport *transport);

protected:
    void signal(Object const * object, int signalIndex, Array && args);

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
};

#endif // BRIDGE_H
