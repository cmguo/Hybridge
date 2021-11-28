#ifndef PAIREDTRANSPORT_H
#define PAIREDTRANSPORT_H

#include "Hybridge_global.h"

#include <core/transport.h>

class HYBRIDGE_EXPORT PairedTransport : public Transport
{
public:
    static std::pair<Transport*, Transport*> createTransports();

private:
    PairedTransport * anothor_;

    // Transport interface
public:
    void sendMessage(Message && message) override;
};

#endif // PAIREDTRANSPORT_H
