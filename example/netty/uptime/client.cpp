#include <blink/TcpClient.h>
#include <blink/Nocopyable.h>
#include <blink/EventLoop.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

class UptimeClient : Nocopyable
{
public:
    UptimeClient(EventLoop* loop, const InetAddress& server_addr)
        : client_(loop, server_addr, "UptimeClient")
    {
        client_.setConnectionCallback(boost::bind(&UptimeClient::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&UptimeClient::onMessage, this, _1, _2, _3));
    }

    void connect()
    {
        client_.connect();
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_TRACE << connection->localAddress().toIpPort() << " -> "
                  << connection->peerAddress().toIpPort() << " is "
                  << (connection->connected() ? "UP" : "DOWN");
    }

    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time)
    {
    }

    TcpClient    client_;
};

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid() << ", tid" << tid();
    EventLoop loop;
    InetAddress server_addr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
    UptimeClient client(&loop, server_addr);
    client.connect();
    loop.loop();
    return 0;
}
