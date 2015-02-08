#include "inspect/Inspector.h"
#include "EventLoop.h"
#include "ProcessInfo.h"
#include "Log.h"

using namespace blink;

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << pid() << ", tid = " << tid();
    EventLoop loop;
    Inspector inspector(&loop, InetAddress(9600), "Inspector");
    loop.loop();
    return 0;
}
