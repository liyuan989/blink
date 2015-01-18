#include "TcpClient.h"
#include "EventLoop.h"
#include "CurrentThread.h"

#include <boost/bind.hpp>

#include <string>
#include <stdio.h>

void ConnectionCallback(const blink::TcpConnectionPtr& connection)
{
    printf("Have connected to %s\n", connection->peerAddress().toIpPort().c_str());
}

int main(int argc, char const *argv[])
{
    blink::EventLoop loop;
    blink::InetAddress server_addr("10.33.1.180", 9600);
    blink::TcpClient client(&loop, server_addr, "TcpClient");
    client.setConnectionCallback(ConnectionCallback);
    client.connect();
    loop.loop();
    printf("main end\n");
    return 0;
}
