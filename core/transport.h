#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "Hybridge_global.h"
#include "message.h"

class Publisher;
class Receiver;

class HYBRIDGE_EXPORT Transport
{
public:
    explicit Transport();

    Transport(Transport const & o) = delete;

    Transport & operator=(Transport const & o) = delete;

    virtual ~Transport();

public:
    virtual void sendMessage(Message &&message) = 0;

    void setPublisher(Publisher * publisher);

    void setReceiver(Receiver * receiver);

protected:
    void messageReceived(Message &&message);

private:
    Publisher * publisher_ = nullptr;
    Receiver * receiver_ = nullptr;
};

#endif // TRANSPORT_H
