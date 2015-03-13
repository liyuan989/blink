#include <example/idleconnection/EchoServer.h>

#include <blink/EventLoop.h>
#include <blink/Log.h>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

void testHash()
{
    boost::hash<boost::shared_ptr<int> > h;
    boost::shared_ptr<int> p1(new int(100));
    boost::shared_ptr<int> p2(new int(100));
    h(p1);
    assert(h(p1) != h(p2));
    p1 = p2;
    assert(h(p1) == h(p2));
    p1.reset();
    assert(h(p1) != h(p2));
    p2.reset();
    assert(h(p1) == h(p2));
}

int main(int argc, char const *argv[])
{
    testHash();
    EventLoop loop;
    InetAddress listen_addr(9600);
    int idle_seconds = 10;
    if (argc > 1)
    {
        idle_seconds = atoi(argv[1]);
    }
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid()
             << "idle seconds = " << idle_seconds;
    EchoServer server(&loop, listen_addr, idle_seconds);
    server.start();
    loop.loop();
    return 0;
}
