#include "EventLoop.h"
#include "Thread.h"
#include "CurrentThread.h"

#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

boost::scoped_ptr<EventLoop> g_loop;

void cb()
{
    printf("cb() pid = %d tid = %d\n", getpid(), tid());
    EventLoop another_loop;
}

void func()
{
    printf("func() pid = %d tid = %d\n", getpid(), tid());
    assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
    EventLoop loop;
    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
    loop.runAfter(1.0, cb);
    loop.loop();
}

int main(int argc, char const *argv[])
{
    printf("main() pid = %d tid = %d\n", getpid(), tid());
    assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
    EventLoop loop;
    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
    Thread thread(func);
    thread.start();
    loop.loop();
    return 0;
}
