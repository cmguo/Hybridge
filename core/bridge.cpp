/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Milian Wolff <milian.wolff@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebChannel module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "bridge.h"
#include "priv/publisher.h"
#include "transport.h"

#include <algorithm>

/*!
    \class Bridge

    \inmodule QtWebChannel
    \brief Exposes Objects to remote HTML clients.
    \since 5.4

    The Bridge fills the gap between C++ applications and HTML/JavaScript
    applications. By publishing a Object derived object to a Bridge and
    using the \l{Qt WebChannel JavaScript API}{Bridge.js} on the HTML side, one can transparently access
    properties and public slots and methods of the Object. No manual message
    passing and serialization of data is required, property updates and signal emission
    on the C++ side get automatically transmitted to the potentially remotely running HTML clients.
    On the client side, a JavaScript object will be created for any published C++ Object. It mirrors the
    C++ object's API and thus is intuitively useable.

    The C++ Bridge API makes it possible to talk to any HTML client, which could run on a local
    or even remote machine. The only limitation is that the HTML client supports the JavaScript
    features used by \c{Bridge.js}. As such, one can interact
    with basically any modern HTML browser or standalone JavaScript runtime, such as node.js.

    There also exists a declarative \l{Qt WebChannel QML Types}{WebChannel API}.

    \sa {Qt WebChannel Standalone Example}, {Qt WebChannel JavaScript API}{JavaScript API}
*/

/*!
    \internal

    Remove a destroyed transport object from the list of known transports.
*/
//void Bridge::_q_transportDestroyed(Object *object)
//{
//    Transport *transport = static_cast<Transport*>(object);
//    const int idx = transports.indexOf(transport);
//    if (idx != -1) {
//        transports.remove(idx);
//        publisher->transportRemoved(transport);
//    }
//}

/*!
    \internal

    Shared code to initialize the Bridge from both constructors.
*/
void Bridge::init()
{
    publisher_ = new Publisher(this);
    //Object::connect(publisher, SIGNAL(blockUpdatesChanged(bool)),
    //                 q, SIGNAL(blockUpdatesChanged(bool)));
}

/*!
    Constructs the Bridge object with the given \a parent.

    Note that a Bridge is only fully operational once you connect it to a
    Transport. The HTML clients also need to be setup appropriately
    using \l{qtwebchannel-javascript.html}{\c Bridge.js}.
*/
Bridge::Bridge()
{
    init();
}

/*!
    Destroys the Bridge.
*/
Bridge::~Bridge()
{
}

/*!
    Registers a group of objects to the Bridge.

    The properties, signals and public invokable methods of the objects are published to the remote clients.
    There, an object with the identifier used as key in the \a objects map is then constructed.

    \note A current limitation is that objects must be registered before any client is initialized.

    \sa Bridge::registerObject(), Bridge::deregisterObject(), Bridge::registeredObjects()
*/
void Bridge::registerObjects(const std::unordered_map< std::string , Object * > &objects)
{
    const std::unordered_map<std::string , Object *>::const_iterator end = objects.cend();
    for (std::unordered_map<std::string , Object *>::const_iterator it = objects.cbegin(); it != end; ++it) {
        publisher_->registerObject(it->first, it->second);
    }
}

/*!
    Returns the map of registered objects that are published to remote clients.

    \sa Bridge::registerObjects(), Bridge::registerObject(), Bridge::deregisterObject()
*/
std::unordered_map<std::string , Object *> Bridge::registeredObjects() const
{
    return publisher_->registeredObjects_;
}

/*!
    Registers a single object to the Bridge.

    The properties, signals and public methods of the \a object are published to the remote clients.
    There, an object with the identifier \a id is then constructed.

    \note A current limitation is that objects must be registered before any client is initialized.

    \sa Bridge::registerObjects(), Bridge::deregisterObject(), Bridge::registeredObjects()
*/
void Bridge::registerObject(const std::string  &id, Object *object)
{
    publisher_->registerObject(id, object);
}

/*!
    Deregisters the given \a object from the Bridge.

    Remote clients will receive a \c destroyed signal for the given object.

    \sa Bridge::registerObjects(), Bridge::registerObject(), Bridge::registeredObjects()
*/
void Bridge::deregisterObject(Object *object)
{
    // handling of deregistration is analogously to handling of a destroyed signal
    Array args;
    args.emplace_back(object);
    publisher_->signalEmitted(object, 0, std::move(args));
}

/*!
    \property Bridge::blockUpdates

    \brief When set to true, updates are blocked and remote clients will not be notified about property changes.

    The changes are recorded and sent to the clients once updates become unblocked again by setting
    this property to false. By default, updates are not blocked.
*/


bool Bridge::blockUpdates() const
{
    return publisher_->blockUpdates_;
}

void Bridge::setBlockUpdates(bool block)
{
    publisher_->setBlockUpdates(block);
}

/*!
    Connects the Bridge to the given \a transport object.

    The transport object then handles the communication between the C++ application and a remote
    HTML client.

    \sa Transport, Bridge::disconnectFrom()
*/
void Bridge::connectTo(Transport *transport)
{
    if (std::find(transports_.begin(), transports_.end(), transport) == transports_.end()) {
        transports_.emplace_back(transport);
        transport->attachBridge(this);
    }
}

/*!
    Disconnects the Bridge from the \a transport object.

    \sa Bridge::connectTo()
*/
void Bridge::disconnectFrom(Transport *transport)
{
    auto idx = std::find(transports_.begin(), transports_.end(), transport);
    if (idx != transports_.end()) {
        transport->attachBridge(nullptr);
        transports_.erase(idx);
        publisher_->transportRemoved(transport);
    }
}

void Bridge::messageReceived(Message &&message, Transport *transport)
{
    publisher_->handleMessage(std::move(message), transport);
}

void Bridge::signal(const Object *from, size_t signalIndex, Array &&args)
{
    publisher_->signalHandler_.dispatch(from, signalIndex, std::move(args));
}

void Bridge::timerEvent()
{
    publisher_->timerEvent();
}

