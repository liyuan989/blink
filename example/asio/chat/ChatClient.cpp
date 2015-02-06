#include "Codec.h"

#include "TcpClient.h"
#include "EventLoopThread.h"
#include "MutexLock.h"
#include "CurrentThread.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <iostream>
#include <stdlib.h>
#include <stdio.h>

using namespace blink;

class ChatClient : Nocopyable
{
public:
    ChatClient(EventLoop* loop, const InetAddress& server_addr)
        : client_(loop, server_addr, "ChatClient"),
          codec_(boost::bind(&ChatClient::onStringMessage, this, _1, _2, _3))
    {
        client_.setConnectionCallback(boost::bind(&ChatClient::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&Codec::onMessage, &codec_, _1, _2, _3));
        client_.enableRetry();
    }

    void connect()
    {
        client_.connect();
    }

    void disconnect()
    {
        client_.disconnect();
    }

    void write(const string& message)
    {
        MutexLockGuard guard(mutex_);
        if (connection_)
        {
            codec_.send(boost::get_pointer(connection_), message);
        }
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_INFO << connection->peerAddress().toIpPort() << " -> "
                 << connection->localAddress().toIpPort() << " is "
                 << (connection->connected() ? "UP" : "DOWN");
        MutexLockGuard guard(mutex_);
        if (connection->connected())
        {
            connection_ = connection;
        }
        else
        {
            connection_.reset();
        }
    }

    void onStringMessage(const TcpConnectionPtr& connection,
                         const string& message,
                         Timestamp receive_time)
    {
        printf("<<< %s\n", message.c_str());
    }

    TcpClient         client_;
    Codec             codec_;
    MutexLock         mutex_;
    TcpConnectionPtr  connection_;
};

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    EventLoopThread loop_thread;
    InetAddress server_addr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
    ChatClient client(loop_thread.startLoop(), server_addr);
    client.connect();
    string line;
    while (std::getline(std::cin, line))
    {
        client.write(line);
    }
    client.disconnect();
    sleepMicroseconds(1000 * 1000);
    return 0;
}
