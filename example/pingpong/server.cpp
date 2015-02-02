#include "TcpServer.h"
#include "EventLoop.h"
#include "Atomic.h"
#include "Thread.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

void onConnection(const TcpConnectionPtr& connection)
{
    if (connection->connected())
    {
        connection->setTcpNoDelay(true);
    }
}

void onMessage(const TcpConnectionPtr& connection,
               Buffer* buf,
               Timestamp receive_time)
{
    connection->send(buf);
}

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <port> <threads>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    Log::setLogLevel(Log::WARN);
    InetAddress listen_addr(static_cast<uint16_t>(atoi(argv[1])));
    EventLoop loop;
    TcpServer server(&loop, listen_addr, "PingPong Server");
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    if (argc == 3)
    {
        server.setThreadNumber(atoi(argv[2]));
    }
    server.start();
    loop.loop();
    return 0;
}

