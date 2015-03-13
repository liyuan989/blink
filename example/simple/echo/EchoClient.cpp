#include <example/simple/echo/EchoClient.h>

#include <blink/CurrentThread.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

EchoClient::EchoClient(EventLoop* loop, const InetAddress& server_addr, const string& name)
    : loop_(loop), client_(loop, server_addr, name)
{
    client_.setConnectionCallback(boost::bind(&EchoClient::onConnection, this, _1));
    client_.setMessageCallback(boost::bind(&EchoClient::onMessage, this,  _1, _2, _3));
}

void EchoClient::connect()
{
    client_.connect();
}

void EchoClient::onConnection(const TcpConnectionPtr& connection)
{
    LOG_TRACE << connection->localAddress().toIpPort() << " -> "
              << connection->peerAddress().toIpPort() << " is "
              << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        connection->send("world\n");
    }
}

void EchoClient::onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp time)
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
