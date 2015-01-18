#include "DaytimeServer.h"

#include "EventLoop.h"
#include "InetAddress.h"
#include "CurrentThread.h"
#include "Log.h"

#include <unistd.h>

using namespace blink;

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    EventLoop loop;
    InetAddress server_addr(9600);
    DaytimeServer server(&loop, server_addr, "DaytimeServer");
    server.start();
    loop.loop();
    return 0;
}
