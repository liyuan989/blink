#include "TcpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "CurrentThread.h"

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

void threadInitCallbac(EventLoop* loop)
{
    printf("thead tid = %d init in %p\n", tid(), loop);
}

void threadConnectionCallnack(const TcpConnectionPtr& connection)
{
    //connection->forceClose();
}

int main(int argc, char const *argv[])
{
    EventLoop loop;
    InetAddress local_addr(9600);
    TcpServer server(&loop, local_addr, "TcpServer");
    server.setThreadNumber(5);
    server.setThreadInitCallback(threadInitCallbac);
    server.setConnectionCallback(threadConnectionCallnack);
    server.start();
    loop.loop();
    return 0;
}
