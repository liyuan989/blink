#include <blink/Nocopyable.h>
#include <blink/TcpServer.h>
#include <blink/EventLoop.h>
#include <blink/Atomic.h>
#include <blink/Log.h>

#include <unistd.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

const int64_t kTimeZoneValue = static_cast<int64_t>(8) * 3600 * 1000 * 1000;

class EchoServer : Nocopyable
{
public:
    EchoServer(EventLoop* loop, const InetAddress& listen_addr, int max_connections)
        : server_(loop, listen_addr, "EchoServer"), kMaxConnections_(max_connections)
    {
        server_.setConnectionCallback(boost::bind(&EchoServer::onConnection, this, _1));
        server_.setMessageCallback(boost::bind(&EchoServer::onMessage, this, _1, _2, _3));
    }

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_INFO << "EchoServer - " << connection->peerAddress().toIpPort() << " -> "
                 << connection->localAddress().toIpPort() << " is "
                 << (connection->connected() ? "UP" : "DOWN");
        if (connection->connected())
        {
            number_connected_.increment();
            if (number_connected_.get() > kMaxConnections_)
            {
                connection->shutdown();
                connection->forceCloseWithDelay(3.0); // > round trip of whole internet.
            }
        }
        else
        {
            number_connected_.decrement();
        }
        LOG_INFO << "number_connected_ = " << number_connected_.get();
    }

    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time)
    {
        string message = buf->resetAllToString();
        int64_t microseconds = receive_time.microSecondsSinceEpoch() + kTimeZoneValue;
        LOG_INFO << connection->name() << " echo " << message.size() << " bytes at "
                 << Timestamp(microseconds).toFormattedString();
        connection->send(message);
    }

    TcpServer    server_;
    AtomicInt32  number_connected_;
    const int    kMaxConnections_;
};

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    int max_connections = 5;
    if (argc > 1)
    {
        max_connections = atoi(argv[1]);
    }
    EventLoop loop;
    InetAddress listen_addr(9600);
    LOG_INFO << "max_connections = " << max_connections;
    EchoServer server(&loop, listen_addr, max_connections);
    server.start();
    loop.loop();
    return 0;
}
