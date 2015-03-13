#include <example/simple/echo/EchoServer.h>

#include <blink/EventLoop.h>
#include <blink/InetAddress.h>
#include <blink/CurrentThread.h>
#include <blink/Log.h>

#include <unistd.h>

using namespace blink;

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    LOG_INFO << "sizeof TcpConnection = " << sizeof(TcpConnection);
    EventLoop loop;
    InetAddress listen_addr(9600);
    EchoServer server(&loop, listen_addr, "EchoServer");
    server.start();
    loop.loop();
    return 0;
}
