#ifndef CHANNEL_H
#define CHANNEL_H

#include "Hybridge_global.h"
#include "meta.h"
#include "message.h"

#include <map>
#include <string>
#include <vector>

class Transport;
class Publisher;
class ProxyObject;

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
    void connectTo(Transport *transport, MetaMethod::Response receive);

    void disconnectFrom(Transport *transport);

protected:
    virtual MetaObject * metaObject(Object const * object) const = 0;

    virtual std::string createUuid() const = 0;

    virtual ProxyObject * createProxyObject() const = 0;

    virtual void startTimer(int msec) = 0;

    virtual void stopTimer() = 0;

protected:
    void messageReceived(Message &&message, Transport *transport);

    void timerEvent();

private:
    void init();

private:
    friend class Publisher;
    friend class MetaObject;
    friend class Receiver;
    friend class Transport;
    friend class SignalHandler;

    Publisher * publisher_;
    std::vector<Transport*> transports_;
    std::map<Transport*, Receiver*> receivers_;
};

#endif // CHANNEL_H
