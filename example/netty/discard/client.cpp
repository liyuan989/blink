#include "TcpClient.h"
#include "Nocopyable.h"
#include "EventLoop.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

class DiscardClient : Nocopyable
{
public:
    DiscardClient(EventLoop* loop, const InetAddress& server_addr, int size)
        : loop_(loop),
          client_(loop, server_addr, "DiscardClient"),
          message_(size, 'H')
    {
        client_.setConnectionCallback(boost::bind(&DiscardClient::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&DiscardClient::onMessage, this, _1, _2, _3));
        client_.setWriteCompleteCallback(boost::bind(&DiscardClient::writeComplete, this, _1));
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
            connection->setTcpNoDelay(true);
            connection->send(message_);
        }
        else
        {
            loop_->quit();
        }
    }

    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time)
    {
        buf->resetAll();
    }

    void writeComplete(const TcpConnectionPtr& connection)
    {
        LOG_INFO << "write complete " << message_.size();
        connection->send(message_);
    }

    EventLoop*   loop_;
    TcpClient    client_;
    string       message_;
};

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid() << ", tid" << tid();
    int size = 512;
    if (argc >= 4)
    {
        size = atoi(argv[3]);
    }
    EventLoop loop;
    InetAddress server_addr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
    DiscardClient client(&loop, server_addr, size);
    client.connect();
    loop.loop();
    return 0;
}
