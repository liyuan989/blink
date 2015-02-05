#include "EchoServer.h"

#include "Log.h"

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

int g_threadNumber = 0;

EchoServer::EchoServer(EventLoop* loop, const InetAddress& listen_addr, const string& name)
    : loop_(loop), server_(loop, listen_addr, name)
{
    server_.setConnectionCallback(boost::bind(&EchoServer::onConnection, this, _1));
    server_.setMessageCallback(boost::bind(&EchoServer::onMessage, this, _1, _2, _3));
    server_.setThreadNumber(g_threadNumber);
}

void EchoServer::start()
{
    server_.start();
}

void EchoServer::onConnection(const TcpConnectionPtr& connection)
{
    LOG_TRACE << connection->peerAddress().toIpPort() << " -> "
              << connection->peerAddress().toIpPort() << " is "
              << (connection->connected() ? "UP" : "DOWN");
    LOG_INFO << connection->getTcpInfoString();
    connection->send("hello\n");
}

void EchoServer::onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp time)
{
    string message = buf->resetAllToString();
    LOG_TRACE << connection->name() << " receive " << message.size()
              << " bytes at " << time.toFormattedString();
    if (message == "exit\n")
    {
        connection->send("bye\n");
        connection->shutdown();
    }
    if (message == "q\n" || message == "quit\n")
    {
        loop_->quit();
    }
    else
    {
        printf("%s\n", message.c_str());
        connection->send(message);
    }
}
