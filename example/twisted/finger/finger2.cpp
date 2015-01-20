#include "EventLoop.h"
#include "TcpServer.h"

int main(int argc, char const *argv[])
{
    blink::EventLoop loop;
    blink::TcpServer server(&loop, blink::InetAddress(9600), "FingerServer");
    server.start();
    loop.loop();
    return 0;
}
