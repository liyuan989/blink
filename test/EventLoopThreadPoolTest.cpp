#include <blink/EventLoopThreadPool.h>
#include <blink/EventLoop.h>
#include <blink/CurrentThread.h>

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdio.h>
#include <assert.h>

using namespace blink;

void print(EventLoop* loop = NULL)
{
    printf("main(): pid = %d, tid = %d, loop = %p\n", getpid(), tid(), loop);
}

void init(EventLoop* loop)
{
    printf("init(): pid = %d, tid = %d, loop = %p\n", getpid(), tid(), loop);
}

int main(int argc, char const *argv[])
{
    print();
    EventLoop loop;
    loop.runAfter(20, boost::bind(&EventLoop::quit, &loop));
    {
        printf("Single thread: %p\n", &loop);
        EventLoopThreadPool loop_thread_pool1(&loop, "thread_pool1");
        loop_thread_pool1.setThreadNumber(0);
        loop_thread_pool1.start(init);
        assert(loop_thread_pool1.getNextLoop() == &loop);
        assert(loop_thread_pool1.getNextLoop() == &loop);
        assert(loop_thread_pool1.getNextLoop() == &loop);
    }
    {
        printf("Another thread:\n");
        EventLoopThreadPool loop_thread_pool2(&loop, "thread_pool2");
        loop_thread_pool2.setThreadNumber(1);
        loop_thread_pool2.start(init);
        EventLoop* next_loop = loop_thread_pool2.getNextLoop();
        next_loop->runAfter(2, boost::bind(print, next_loop));
        assert(&loop != next_loop);
        assert(next_loop == loop_thread_pool2.getNextLoop());
        assert(next_loop == loop_thread_pool2.getNextLoop());
        sleep(3);
    }
    {
        printf("Three threads:\n");
        EventLoopThreadPool loop_thread_pool3(&loop, "thread_pool3");
        loop_thread_pool3.setThreadNumber(3);
        loop_thread_pool3.start(init);
        EventLoop* next_loop = loop_thread_pool3.getNextLoop();
        assert(&loop != next_loop);
        assert(next_loop != loop_thread_pool3.getNextLoop());
        assert(next_loop != loop_thread_pool3.getNextLoop());
        assert(next_loop == loop_thread_pool3.getNextLoop());
        (void)next_loop;
    }
    loop.loop();
    return 0;
}
