#include <blink/TcpClient.h>
#include <blink/EventLoop.h>
#include <blink/InetAddress.h>
#include <blink/CurrentThread.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>

using namespace blink;

class Client
{
public:
    Client(EventLoop* loop, const InetAddress& server_addr, const string& name)
        : loop_(loop), client_(loop, server_addr, name)
    {
        client_.setConnectionCallback(boost::bind(&Client::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&Client::onMessage, this,  _1, _2, _3));
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
        if (!connection->connected())
        {
            loop_->quit();
        }
    }

    void onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp time)
    {
        string message = buf->resetAllToString();
        LOG_INFO << "Server time: " << message;
        connection->shutdown();
    }

    EventLoop*  loop_;
    TcpClient   client_;
};

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << ::getpid() << ", tid" << current_thread::tid();
    EventLoop loop;
    InetAddress server_addr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
    Client client(&loop, server_addr, "Client");
    client.connect();
    loop.loop();
    return 0;
}
