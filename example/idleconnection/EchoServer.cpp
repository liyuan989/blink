#include "EchoServer.h"

#include "EventLoop.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <assert.h>
#include <stdio.h>

using namespace blink;

const int64_t kTimeZoneValue = static_cast<int64_t>(8) * 3600 * 1000 * 1000;

EchoServer::EchoServer(EventLoop* loop,
                       const InetAddress& listen_addr,
                       int idle_seconds)
    : server_(loop, listen_addr, "EchoServer"),
      connection_buckets_(idle_seconds)
{
    server_.setConnectionCallback(boost::bind(&EchoServer::onConnection, this, _1));
    server_.setMessageCallback(boost::bind(&EchoServer::onMessage, this, _1, _2, _3));
    loop->runEvery(1.0, boost::bind(&EchoServer::onTimer, this));
    connection_buckets_.resize(idle_seconds);
    dumpConnectionBuckets();
}

void EchoServer::start()
{
    server_.start();
}

void EchoServer::onConnection(const TcpConnectionPtr& connection)
{
    LOG_INFO << "EchoServer - " << connection->peerAddress().toIpPort() << " -> "
             << connection->localAddress().toIpPort() << " is "
             << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        EntryPtr entry(new Entry(connection));
        connection_buckets_.back().insert(entry);
        dumpConnectionBuckets();
        WeakEntryPtr weak_entry(entry);
        connection->setContext(weak_entry);  // use weak_ptr to avoid circular reference.
    }
    else
    {
        assert(!connection->getContext().empty());
        WeakEntryPtr weak_entry(boost::any_cast<WeakEntryPtr>(connection->getContext()));
        LOG_DEBUG << "Entry use_count = " << weak_entry.use_count();
    }
}

void EchoServer::onMessage(const TcpConnectionPtr& connection,
                           Buffer* buf,
                           Timestamp receive_time)
{
    std::string message = buf->resetAllToString();
    LOG_INFO << connection->name() << " echo " << message.size() << " bytes at "
             << Timestamp(receive_time.microSecondsSinceEpoch() + kTimeZoneValue).toFormattedString();
    connection->send(message);
    assert(!connection->getContext().empty());
    WeakEntryPtr weak_entry(boost::any_cast<WeakEntryPtr>(connection->getContext()));
    EntryPtr entry(weak_entry.lock());
    if (entry)
    {
        connection_buckets_.back().insert(entry);
        dumpConnectionBuckets();
    }
}

void EchoServer::onTimer()
{
    connection_buckets_.push_back(Bucket());
    dumpConnectionBuckets();
}

void EchoServer::dumpConnectionBuckets() const
{
    LOG_INFO << "size = " << connection_buckets_.size();
    int index = 0;
    for (WeakConnectionList::const_iterator iter = connection_buckets_.begin();
         iter != connection_buckets_.end(); ++iter, ++index)
    {
        const Bucket& bucket = *iter;
        printf("[%d] len = %zd\n", index, bucket.size());
        for (Bucket::const_iterator it = bucket.begin(); it != bucket.end(); ++it)
        {
            bool connection_dead = (*it)->weak_connection_.expired();
            printf("%p(%ld)%s\n", boost::get_pointer(*it), it->use_count(),
                   connection_dead ? " DEAD" : " ");
        }
        puts("");
    }
}
