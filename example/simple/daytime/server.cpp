#include <example/simple/daytime/DaytimeServer.h>

#include <blink/EventLoop.h>
#include <blink/InetAddress.h>
#include <blink/CurrentThread.h>
#include <blink/Log.h>

#include <unistd.h>

using namespace blink;

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << current_thread::tid();
    EventLoop loop;
    InetAddress server_addr(9600);
    DaytimeServer server(&loop, server_addr, "DaytimeServer");
    server.start();
    loop.loop();
    return 0;
}
