#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include "core/meta.h"

#include <unordered_map>
#include <vector>

class Publisher;

class SignalHandler
{
public:
    SignalHandler(Publisher *receiver);

    /**
     * Connect to a signal of @p object identified by @p signalIndex.
     *
     * If the handler is already connected to the signal, an internal counter is increased,
     * i.e. the handler never connects multiple times to the same signal.
     */
    void connectTo(const Object *object, size_t signalIndex);

    /**
     * Decrease the connection counter for the connection to the given signal.
     *
     * When the counter drops to zero, the connection is disconnected.
     */
    void disconnectFrom(const Object *object, size_t signalIndex);

    /**
     * @internal
     *
     * Custom implementation of qt_metacall which calls dispatch() for connected signals.
     */
    //int qt_metacall(QMetaObject::Call call, int methodId, void **args) override;

    /**
     * Reset all connections, useful for benchmarks.
     */
    void clear();

    /**
     * Fully remove and disconnect an object from handler
     */
    void remove(const Object *object);

    /**
     * Exctract the arguments of a signal call and pass them to the receiver.
     *
     * The @p argumentData is converted to a QVariantList and then passed to the receiver's
     * signalEmitted method.
     */
    void dispatch(const Object *object, size_t signalIdx, Array && argumentData);

private:
    void setupSignalArgumentTypes(const MetaObject *metaObject, const MetaMethod &signal);

    Publisher *publisher_;

    // maps meta object -> signalIndex -> list of arguments
    // NOTE: This data is "leaked" on disconnect until deletion of the handler, is this a problem?
    typedef std::vector<int> ArgumentTypeList;
    typedef std::unordered_map<size_t, ArgumentTypeList> SignalArgumentHash;
    std::unordered_map<const MetaObject *, SignalArgumentHash > m_signalArgumentTypes;

    /*
     * Tracks how many connections are active to object signals.
     *
     * Maps object -> signalIndex -> pair of connection and number of connections
     *
     * Note that the handler is connected to the signal only once, whereas clients
     * may have connected multiple times.
     *
     * TODO: Move more of this logic to the HTML client side, esp. the connection counting.
     */
    typedef std::pair<MetaObject::Connection, int> ConnectionPair;
    typedef std::unordered_map<size_t, ConnectionPair> SignalConnectionHash;
    typedef std::unordered_map<const Object*, SignalConnectionHash> ConnectionHash;
    ConnectionHash m_connectionsCounter;
};

#endif // SIGNALHANDLER_H
