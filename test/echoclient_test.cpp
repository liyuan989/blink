#include "TcpClient.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "CurrentThread.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>

using namespace blink;

class EchoClient
{
public:
    EchoClient(EventLoop* loop, const InetAddress& server_addr, const string& name)
        : loop_(loop), client_(loop, server_addr, name)
    {
        client_.setConnectionCallback(boost::bind(&EchoClient::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&EchoClient::onMessage, this,  _1, _2, _3));
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
        if (connection->connected())
        {
            connection->send("world\n");
        }
    }

    void onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp time)
    {
        string message = buf->resetAllToString();
        LOG_TRACE << connection->name() << " receive " << message.size()
                  << " bytes at " << time.toFormattedString();
        if (message == "q\n" || message == "quit\n")
        {
            connection->send("bye");
            connection->shutdown();
        }
        else if (message == "shutdown\n")
        {
            loop_->quit();
        }
        else
        {
            printf("%s\n", message.c_str());
            connection->send(message);
        }
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
    LOG_INFO << "pid = " << ::getpid() << ", tid" << tid();
    EventLoop loop;
    InetAddress server_addr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
    EchoClient client(&loop, server_addr, "EchoClient");
    client.connect();
    loop.loop();
    return 0;
}
