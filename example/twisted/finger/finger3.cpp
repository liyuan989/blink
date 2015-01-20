#include "EventLoop.h"
#include "TcpServer.h"

using namespace blink;

void onConnection(const TcpConnectionPtr& connection)
{
    if (connection->connected())
    {
        connection->shutdown();
    }
}

int main(int argc, char const *argv[])
{
    EventLoop loop;
    TcpServer server(&loop, InetAddress(9600), "FingerServer");
    server.setConnectionCallback(onConnection);
    server.start();
    loop.loop();
    return 0;
}
