#include "DaytimeServer.h"

#include "Nocopyable.h"
#include "TcpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "CurrentThread.h"
#include "Log.h"

#include <boost/bind.hpp>

using namespace blink;

const int64_t kTimeZoneValue = static_cast<int64_t>(8) * 3600 * 1000 * 1000;

DaytimeServer::DaytimeServer(EventLoop* loop,
                             const InetAddress& server_addr,
                             const string& name)
    : loop_(loop), server_(loop, server_addr, name)
{
    server_.setConnectionCallback(boost::bind(&DaytimeServer::onConnection, this, _1));
    server_.setMessageCallback(boost::bind(&DaytimeServer::onMessage, this, _1, _2, _3));
}

void DaytimeServer::start()
{
    server_.start();
}

void DaytimeServer::onConnection(const TcpConnectionPtr& connection)
{
    LOG_INFO << "DaytimeServer - " << connection->peerAddress().toIpPort() << " -> "
             << connection->localAddress().toIpPort() << " is "
             << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        Timestamp now(Timestamp::now().microSecondsSinceEpoch() + kTimeZoneValue);
        connection->send(now.toFormattedString());
        connection->shutdown();
    }
}

void DaytimeServer::onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp time)
{
    string message = buf->resetAllToString();
    LOG_INFO << connection->name() << " discards " << message.size() << " bytes received at "
             << Timestamp(time.microSecondsSinceEpoch() + kTimeZoneValue).toFormattedString();
}
