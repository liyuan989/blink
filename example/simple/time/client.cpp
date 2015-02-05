#include "TcpClient.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "CurrentThread.h"
#include "SocketBase.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>

using namespace blink;
using namespace blink::sockets;

const int64_t kTimeZoneValue = static_cast<int64_t>(8) * 3600 * 1000 * 1000;

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
        if (buf->readableSize() >= sizeof(int64_t))
        {
            int64_t server_time_since_epoch = networkToHost64(buf->readInt64());
            LOG_INFO << "Server time: " << Timestamp(server_time_since_epoch).toFormattedString();
        }
        else
        {
            LOG_INFO << connection->name() << " no enough data " << buf->readableSize() << " at "
                     << Timestamp(Timestamp::now().microSecondsSinceEpoch() + kTimeZoneValue).toFormattedString();
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
    Client client(&loop, server_addr, "Client");
    client.connect();
    loop.loop();
    return 0;
}
