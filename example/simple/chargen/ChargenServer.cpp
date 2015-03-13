#include <example/simple/chargen/ChargenServer.h>

#include <blink/CurrentThread.h>
#include <blink/Timestamp.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

const int64_t kTimeZoneValue = static_cast<int64_t>(8) * 3600 * 1000 * 1000;

ChargenServer::ChargenServer(EventLoop* loop,
                             const InetAddress& server_addr,
                             const string& name,
                             bool print)
    : loop_(loop),
      server_(loop, server_addr, name),
      transferred_(0),
      start_time_(Timestamp::now().microSecondsSinceEpoch() + kTimeZoneValue)
{
    server_.setConnectionCallback(boost::bind(&ChargenServer::onConnection, this, _1));
    server_.setMessageCallback(boost::bind(&ChargenServer::onMessage, this, _1, _2, _3));
    server_.setWriteCompleteCallback(boost::bind(&ChargenServer::onWriteComplete, this, _1));
    if (print)
    {
        loop->runEvery(3.0, boost::bind(&ChargenServer::printThroughput, this));
    }
    string line;
    for (int i = 33; i < 127; ++i)
    {
        line.push_back(char(i));
    }
    line += line;
    for (size_t i = 0; i < 127 - 33; ++i)
    {
        message_ += line.substr(i, 72) + '\n';
    }
}

void ChargenServer::start()
{
    server_.start();
}

void ChargenServer::onConnection(const TcpConnectionPtr& connection)
{
    LOG_INFO << "ChargenServer - " << connection->peerAddress().toIpPort()
             << " -> " << connection->localAddress().toIpPort() << " is "
             << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        connection->setTcpNoDelay(true);
        connection->send(message_);
    }
}

void ChargenServer::onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp time)
{
    string message = buf->resetAllToString();
    LOG_INFO << connection->name() << " discards " << message.size() << " bytes received at "
             << Timestamp(time.microSecondsSinceEpoch() + kTimeZoneValue).toFormattedString();
}

void ChargenServer::onWriteComplete(const TcpConnectionPtr& connection)
{
    transferred_ += message_.size();
    connection->send(message_);
}

void ChargenServer::printThroughput()
{
    Timestamp end_time(Timestamp::now().microSecondsSinceEpoch() + kTimeZoneValue);
    double time = timeDifference(end_time, start_time_);
    printf("%4.3f MiB/s\n", static_cast<double>(transferred_) / time / 1024 / 1024);
}
