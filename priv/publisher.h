#ifndef PUBLISHER_H
#define PUBLISHER_H

#include "core/value.h"
#include "core/object.h"
#include "signalhandler.h"
#include "core/message.h"

#include <set>

// NOTE: keep in sync with corresponding maps in Bridge.js and WebChannelTest.qml

class Channel;
class Transport;
class SignalHandler;
class Publisher;

class Publisher
{
public:
    explicit Publisher(Channel *bridge);
    virtual ~Publisher();

    /**
     * Register @p object under the given @p id.
     *
     * The properties, signals and public methods of the Object are
     * published to the remote client, where an object with the given identifier
     * is constructed.
     *
     * TODO: This must be called, before clients are initialized.
     */
    void registerObject(const std::string &id, Object *object);

    /**
     * Send the given message to all known transports.
     */
    void broadcastMessage(Message const &message) const;

    /**
     * Serialize the QMetaObject of @p object and return it in JSON form.
     */
    Map classInfoForObject(Object const *object, Transport *transport);

    /**
     * Set the client to idle or busy, based on the value of @p isIdle.
     *
     * When the value changed, start/stop the property update timer accordingly.
     */
    void setClientIsIdle(bool isIdle);

    /**
     * Initialize clients by sending them the class information of the registered objects.
     *
     * Furthermore, if that was not done already, connect to their property notify signals.
     */
    Map initializeClient(Transport *transport);

    /**
     * Go through all properties of the given object and connect to their notify signal.
     *
     * When receiving a notify signal, it will store the information in pendingPropertyUpdates which
     * gets send via a Qt.propertyUpdate message to the server when the grouping timer timeouts.
     */
    void initializePropertyUpdates(Object const * const object, Map const &objectInfo);

    /**
     * Send the clients the new property values since the last time this function was invoked.
     *
     * This is a grouped batch of all properties for which their notify signal was emitted.
     * The list of signals as well as the arguments they contained, are also transmitted to
     * the remote clients.
     *
     * @sa timer, initializePropertyUpdates
     */
    void sendPendingPropertyUpdates();

    /**
     * Invoke the method of index @p methodIndex on @p object with the arguments @p args.
     *
     * The return value of the method invocation is then serialized and a response message
     * is returned.
     */
    Value invokeMethod(Object *const object, size_t methodIndex, Array &&args);

    /**
     * Set the value of property @p propertyIndex on @p object to @p value.
     */
    void setProperty(Object *object, size_t propertyIndex, Value &&value);

    /**
     * Callback of the signalHandler which forwards the signal invocation to the webchannel clients.
     */
    void signalEmitted(Object const *object, size_t signalIndex, Array &&arguments);

    /**
     * Callback for registered or wrapped objects which erases all data related to @p object.
     *
     * @sa signalEmitted
     */
    void objectDestroyed(Object const *object);

    Object *unwrapObject(std::string const &objectId) const;

    Value toVariant(Value &&value, int targetType) const;

    /**
     * Remove wrapped objects which last transport relation is with the passed transport object.
     */
    void transportRemoved(Transport *transport);

    /**
     * Given a Variant containing a Object*, wrap the object and register for property updates
     * return the objects class information.
     *
     * All other input types are returned as-is.
     */
    Value wrapResult(Value &&result, Transport *transport,
                          std::string const &parentObjectId = std::string());

    /**
     * Convert a list of variant values for consumption by the client.
     *
     * This properly handles QML values and also wraps the result if required.
     */
    Array wrapList(Array &list, Transport *transport,
                          std::string const &parentObjectId = std::string());

    /**
     * Invoke delete later on @p object.
     */
    void deleteWrappedObject(Object *object) const;

    /**
     * When updates are blocked, no property updates are transmitted to remote clients.
     */
    void setBlockUpdates(bool block);

//Q_SIGNALS:
//    void blockUpdatesChanged(bool block);

public:
    /**
     * Handle the @p message and if needed send a response to @p transport.
     */
    void handleMessage(Message &&message, Transport *transport);

    void propertyChanged(Object const * object, size_t propertyIndex);

protected:
    void timerEvent();

private:
    friend class Channel;
    friend class TestBridge;
    friend class SignalHandler;

    Channel * channel_;
    SignalHandler signalHandler_;

    // true when the client is idle, false otherwise
    bool clientIsIdle_;

    // true when no property updates should be sent, false otherwise
    bool blockUpdates_;

    // true when at least one client was initialized and thus
    // the property updates have been initialized and the
    // object info map set.
    bool propertyUpdatesInitialized_;

    // Map of registered objects indexed by their id.
    std::unordered_map<std::string, Object *> registeredObjects_;

    // Map the registered objects to their id.
    std::unordered_map<const Object *, std::string> registeredObjectIds_;

    // Groups individually wrapped objects with their class information and the transports that have access to it.
    struct ObjectInfo
    {
        ObjectInfo()
            : object(nullptr)
        {}
        ObjectInfo(Object *o, Value &&i)
            : object(o)
            , classinfo(std::move(i))
        {}
        ObjectInfo(ObjectInfo const &) = delete;
        ObjectInfo(ObjectInfo && o)
            : object(o.object)
            , classinfo(std::move(o.classinfo))
        {}
        Object *object;
        Value classinfo;
        std::vector<Transport*> transports;
    };

    // Map of objects wrapped from invocation returns
    std::unordered_map<std::string, ObjectInfo> wrappedObjects_;
    // Map of transports to wrapped object ids
    std::unordered_multimap<Transport*, std::string> transportedWrappedObjects_;

    // Map of objects to maps of signal indices to a set of all their property indices.
    // The last value is a set as a signal can be the notify signal of multiple properties.
    typedef std::unordered_map<size_t, std::set<size_t> > SignalToPropertyNameMap;
    std::unordered_map<const Object *, SignalToPropertyNameMap> signalToPropertyMap_;

    // Objects that changed their properties and are waiting for idle client.
    // map of object name to map of signal index to arguments
    typedef std::unordered_map<size_t, Value> SignalToArgumentsMap;
    typedef std::unordered_map<Object const *, SignalToArgumentsMap> PendingPropertyUpdates;
    PendingPropertyUpdates pendingPropertyUpdates_;

    std::unordered_map<Object const *, std::set<size_t> > pendingPropertyUpdates2_;

    // Aggregate property updates since we get multiple Qt.idle message when we have multiple
    // clients. They all share the same QWebProcess though so we must take special care to
    // prevent message flooding.
    //QBasicTimer timer;
};


#endif // QMETAOBJECTPUBLISHER_P_H
