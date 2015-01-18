#include "EventLoop.h"
#include "EventLoopThread.h"
#include "Thread.h"
#include "CountDownLatch.h"
#include "CurrentThread.h"

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

void print(EventLoop* p = NULL)
{
    printf("print: pid = %d, tid = %d. loop = %p\n", getpid(), tid(), p);
}

void quit(EventLoop* p)
{
    print(p);
    p->quit();
}

int main(int argc, char const *argv[])
{
    print();
    {
        EventLoopThread loop_thread1;
    }
    {   // destructor call quit()
        EventLoopThread loop_thread2;
        EventLoop* loop = loop_thread2.startLoop();
        loop->runInLoop(boost::bind(print, loop));
        sleepMicrosecond(500 * 1000);
    }
    {   // call quit() before destruct.
        EventLoopThread loop_thread3;
        EventLoop* loop = loop_thread3.startLoop();
        loop->runInLoop(boost::bind(quit, loop));
        sleepMicrosecond(500 * 1000);
    }
    return 0;
}
