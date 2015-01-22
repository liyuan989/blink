#ifndef __EXAMPLE_IDLECONNECTION_ECHOSERVER_H__
#define __EXAMPLE_IDLECONNECTION_ECHOSERVER_H__

#include "Nocopyable.h"
#include "TcpServer.h"

#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>

#if BOOST_VERSION < 104700

namespace boost
{

template<typename T>
inline size_t hash_value(const shared_ptr<T>& p)
{
    return hash_value(p.get());
}

}  // namespace boost

#endif

class EchoServer : blink::Nocopyable
{
public:
    EchoServer(blink::EventLoop* loop,
               const blink::InetAddress& listen_addr,
               int idle_seconds);

    void start();

private:
    void onConnection(const blink::TcpConnectionPtr& connection);
    void onMessage(const blink::TcpConnectionPtr& connection,
                   blink::Buffer* buf,
                   blink::Timestamp receive_time);
    void onTimer();
    void dumpConnectionBuckets() const;

    typedef boost::weak_ptr<blink::TcpConnection> WeakTcpConnectionPtr;

    class Entry
    {
    public:
        Entry(const WeakTcpConnectionPtr& weak_connection)
            : weak_connection_(weak_connection)
        {
        }

        ~Entry()
        {
            blink::TcpConnectionPtr connection = weak_connection_.lock();
            if (connection)
            {
                connection->shutdown();
            }
        }

        WeakTcpConnectionPtr weak_connection_;
    };

    typedef boost::shared_ptr<Entry> EntryPtr;
    typedef boost::weak_ptr<Entry> WeakEntryPtr;
    typedef boost::unordered_set<EntryPtr> Bucket;
    typedef boost::circular_buffer<Bucket> WeakConnectionList;

    blink::TcpServer    server_;
    WeakConnectionList  connection_buckets_;
};

#endif
