#include "transport.h"
#include "bridge.h"

/*!
    \class Transport

    \inmodule QtWebChannel
    \brief Communication channel between the C++ QWebChannel server and a HTML/JS client.
    \since 5.4

    Users of the QWebChannel must implement this interface and connect instances of it
    to the QWebChannel server for every client that should be connected to the QWebChannel.
    The \l{Qt WebChannel Standalone Example} shows how this can be done
    using \l{Qt WebSockets}.

    \note The JSON message protocol is considered internal and might change over time.

    \sa {Qt WebChannel Standalone Example}
*/

/*!
    \fn Transport::messageReceived(const QJsonObject &message, Transport *transport)

    This signal must be emitted when a new JSON \a message was received from the remote client. The
    \a transport argument should be set to this transport object.
*/

/*!
    \fn Transport::sendMessage(const QJsonObject &message)

    Sends a JSON \a message to the remote client. An implementation would serialize the message and
    transmit it to the remote JavaScript client.
*/

/*!
    Constructs a transport object with the given \a parent.
*/
Transport::Transport()
{
}

/*!
    Destroys the transport object.
*/
Transport::~Transport()
{
    if (bridge_) {
        bridge_->disconnectFrom(this);
    }
}

void Transport::attachBridge(Bridge *bridge)
{
    bridge_ = bridge;
}
