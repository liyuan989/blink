#include "TcpClient.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "CurrentThread.h"
#include "SocketBase.h"
#include "Atomic.h"
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
        client_.setWriteCompleteCallback(boost::bind(&Client::onWriteComplete, this, _1));
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
        else
        {
            connection->send("hey boy!");
            num_.increment();
        }
    }

    void onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp time)
    {

        string message = buf->resetAllToString();
        LOG_INFO << connection->name() << " discards " << message.size() << " bytes received at "
             << Timestamp(time.microSecondsSinceEpoch() + kTimeZoneValue).toFormattedString();
    }

    void onWriteComplete(const TcpConnectionPtr& connection)
    {
        if (num_.get() < 20)
        {
            connection->send("hey boy!");
            num_.increment();
        }
        else
        {
            connection->shutdown();
        }
    }

    EventLoop*   loop_;
    TcpClient    client_;
    AtomicInt32  num_;
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
