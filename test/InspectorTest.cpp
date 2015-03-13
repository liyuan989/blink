#include <blink/inspect/Inspector.h>
#include <blink/EventLoop.h>
#include <blink/ProcessInfo.h>
#include <blink/Log.h>

using namespace blink;

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << pid() << ", tid = " << tid();
    EventLoop loop;
    Inspector inspector(&loop, InetAddress(9600), "Inspector");
    loop.loop();
    return 0;
}
