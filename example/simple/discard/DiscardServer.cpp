#include <example/simple/discard/DiscardServer.h>

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

DiscardServer::DiscardServer(EventLoop* loop,
                             const InetAddress& server_addr,
                             const string& name)
    : loop_(loop), server_(loop, server_addr, name)
{
    server_.setConnectionCallback(boost::bind(&DiscardServer::onConnection, this, _1));
    server_.setMessageCallback(boost::bind(&DiscardServer::onMessage, this, _1, _2, _3));
}

void DiscardServer::start()
{
    server_.start();
}

void DiscardServer::onConnection(const TcpConnectionPtr& connection)
{
    LOG_INFO << "DiscardServer - " << connection->peerAddress().toIpPort() << " -> "
             << connection->localAddress().toIpPort() << " is "
             << (connection->connected() ? "UP" : "DOWN");
}

void DiscardServer::onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp time)
{
    string message = buf->resetAllToString();
    LOG_INFO << connection->name() << " discards " << message.size() << " bytes received at "
             << Timestamp(time.microSecondsSinceEpoch() + kTimeZoneValue).toFormattedString();
}

