#include <example/simple/time/TimeServer.h>

#include <blink/Nocopyable.h>
#include <blink/TcpServer.h>
#include <blink/EventLoop.h>
#include <blink/InetAddress.h>
#include <blink/CurrentThread.h>
#include <blink/SocketBase.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

using namespace blink;
using namespace blink::sockets;

const int64_t kTimeZoneValue = static_cast<int64_t>(8) * 3600 * 1000 * 1000;

TimeServer::TimeServer(EventLoop* loop,
                             const InetAddress& server_addr,
                             const string& name)
    : loop_(loop), server_(loop, server_addr, name)
{
    server_.setConnectionCallback(boost::bind(&TimeServer::onConnection, this, _1));
    server_.setMessageCallback(boost::bind(&TimeServer::onMessage, this, _1, _2, _3));
}

void TimeServer::start()
{
    server_.start();
}

void TimeServer::onConnection(const TcpConnectionPtr& connection)
{
    LOG_INFO << "TimeServer - " << connection->peerAddress().toIpPort() << " -> "
             << connection->localAddress().toIpPort() << " is "
             << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        Timestamp time_now(Timestamp::now().microSecondsSinceEpoch() + kTimeZoneValue);
        int64_t now = hostToNetwork64(time_now.microSecondsSinceEpoch());
        connection->send(&now, sizeof(now));
        connection->shutdown();
    }
}

void TimeServer::onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp time)
{
    string message = buf->resetAllToString();
    LOG_INFO << connection->name() << " discards " << message.size() << " bytes received at "
             << Timestamp(time.microSecondsSinceEpoch() + kTimeZoneValue).toFormattedString();
}
