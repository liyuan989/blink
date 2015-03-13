#include <example/simple/chargen/ChargenClient.h>

#include <blink/EventLoop.h>
#include <blink/InetAddress.h>
#include <blink/CurrentThread.h>
#include <blink/Log.h>

#include <unistd.h>

#include <stdlib.h>

using namespace blink;


int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    EventLoop loop;
    InetAddress server_addr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
    ChargenClient client(&loop, server_addr, "ChargenClient");
    client.connect();
    loop.loop();
    return 0;
}
