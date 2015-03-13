#include <blink/TimerQueue.h>
#include <blink/EventLoopThread.h>
#include <blink/EventLoop.h>
#include <blink/Thread.h>
#include <blink/CurrentThread.h>
#include <blink/TimerId.h>

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

int count = 0;
EventLoop* g_loop;

void printId()
{
    printf("pid = %d, tid = %d\n", getpid(), tid());
    printf("now %s\n", Timestamp::now().toFormattedString().c_str());
}

void print(const char* message)
{
    printf("message %s %s\n", Timestamp::now().toFormattedString().c_str(), message);
    if (++count == 100)
    {
        g_loop->quit();
    }
}

void cancel(TimerId timer)
{
    g_loop->cancel(timer);
    printf("canceled at %s\n", Timestamp::now().toFormattedString().c_str());
}

int main(int argc, char const *argv[])
{
    printId();
    sleep(1);
    {
        EventLoop loop;
        g_loop = &loop;
        printf("main\n");
        loop.runAfter(1, boost::bind(print, "once1"));
        loop.runAfter(1.5, boost::bind(print, "once1.5"));
        loop.runAfter(2.5, boost::bind(print, "once2.5"));
        loop.runAfter(3.5, boost::bind(print, "once3.5"));
        TimerId t45 = loop.runAfter(4, boost::bind(print, "once4.5"));
        loop.runAfter(4.2, boost::bind(cancel, t45));
        loop.runAfter(4.8, boost::bind(cancel, t45));

        loop.runEvery(2, boost::bind(print, "every2"));
        TimerId t3 = loop.runEvery(3, boost::bind(print, "every3"));
        loop.runAfter(9.001, boost::bind(cancel, t3));
        printf("loop start\n");
        loop.loop();
        printf("main loop exits.\n");
    }
    sleep(1);
    {
        EventLoopThread loop_thread;
        EventLoop* loop = loop_thread.startLoop();
        loop->runAfter(2, printId);
        sleep(3);
        printf("thread loop exits.\n");
    }
    return 0;
}

