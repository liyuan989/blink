#include <example/simple/chargen/ChargenClient.h>

#include <blink/Log.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

ChargenClient::ChargenClient(EventLoop* loop, const InetAddress& server_addr, const string& name)
    : loop_(loop), client_(loop, server_addr, name)
{
    client_.setConnectionCallback(boost::bind(&ChargenClient::onConnection, this, _1));
    client_.setMessageCallback(boost::bind(&ChargenClient::onMessage, this, _1, _2, _3));
}

void ChargenClient::connect()
{
    client_.connect();
}

void ChargenClient::onConnection(const TcpConnectionPtr& connection)
{
    LOG_INFO << connection->localAddress().toIpPort() << " -> "
             << connection->peerAddress().toIpPort() << " is "
             << (connection->connected() ? "UP" : "DOWN");
    if (!connection->connected())
    {
        loop_->quit();
    }
}

void ChargenClient::onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp time)
{
    string msg = buf->resetAllToString();
    printf("received data size = %zd bytes.\n", msg.size());
}
