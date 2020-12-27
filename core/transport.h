#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "Hybridge_global.h"
#include "message.h"

class Channel;

class HYBRIDGE_EXPORT Transport
{
public:
    explicit Transport();

    Transport(Transport const & o) = delete;

    Transport & operator=(Transport const & o) = delete;

    virtual ~Transport();

public:
    virtual void sendMessage(const Message &message) = 0;

    void attachBridge(Channel * bridge);

protected:
    void messageReceived(Message &&message);

private:
    Channel * bridge_ = nullptr;
};

#endif // TRANSPORT_H
