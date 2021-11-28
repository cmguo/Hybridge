#include "pairedtransport.h"

#include <iostream>

std::pair<Transport*, Transport*> PairedTransport::createTransports()
{
    auto pair = std::make_pair(new PairedTransport, new PairedTransport);
    pair.first->anothor_ = pair.second;
    pair.second->anothor_ = pair.first;
    return pair;
}

void PairedTransport::sendMessage(Message &&message)
{
    std::cout << message << std::endl;
    anothor_->messageReceived(std::move(message));
}
