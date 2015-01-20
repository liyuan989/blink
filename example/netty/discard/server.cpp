#include "TcpServer.h"
#include "Nocopyable.h"
#include "EventLoop.h"
#include "Atomic.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

int g_numberThreads = 0;

class DiscardServer : Nocopyable
{
public:
    DiscardServer(EventLoop* loop, const InetAddress& server_addr)
        : server_(loop, server_addr, "DiscardServer"),
          old_counter_(0),
          start_time_(Timestamp::now())
    {
        server_.setConnectionCallback(boost::bind(&DiscardServer::onConnection, this, _1));
        server_.setMessageCallback(boost::bind(&DiscardServer::onMessage, this, _1, _2, _3));
        server_.setThreadNumber(g_numberThreads);
        loop->runEvery(3.0, boost::bind(&DiscardServer::printThroughput, this));
    }

    void start()
    {
        LOG_INFO << "starting " << g_numberThreads << " threads.";
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_INFO << "DiscardServer - " << connection->peerAddress().toIpPort() << " -> "
                 << connection->localAddress().toIpPort() << " is "
                 << (connection->connected() ? "UP" : "DOWN");
    }

    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time)
    {
        size_t len = buf->readableSize();
        transferred_.add(len);
        received_messages_.incrementAndGet();
        buf->resetAll();
    }

    void printThroughput()
    {
        Timestamp end_time = Timestamp::now();
        int64_t new_counter = transferred_.get();
        int64_t bytes = new_counter - old_counter_;
        int64_t messages = received_messages_.getAndSet(0);
        double time = timeDifference(end_time, start_time_);
        printf("%4.3f MiB/s %4.3f Ki Msg/s %6.2f bytes/msg\n",
               static_cast<double>(bytes) / time / 1024 / 1024,
               static_cast<double>(messages) / time / 1024,
               static_cast<double>(bytes) / static_cast<double>(messages));
        old_counter_ = new_counter;
        start_time_ = end_time;
    }

    TcpServer    server_;
    AtomicInt64  transferred_;
    AtomicInt64  received_messages_;
    int64_t      old_counter_;
    Timestamp    start_time_;
};

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    if (argc > 1)
    {
        g_numberThreads = atoi(argv[1]);
    }
    EventLoop loop;
    DiscardServer server(&loop, InetAddress(9600));
    server.start();
    loop.loop();
    return 0;
}
