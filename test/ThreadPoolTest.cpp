#include "ThreadPool.h"
#include "CurrentThread.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <time.h>
#include <string>
#include <stdio.h>

using namespace blink;

void print()
{
    LOG_INFO << "tid=" << tid();
}

void printString(const std::string& s)
{
    LOG_INFO << "tid=" << tid() << " " << s.c_str();
}

void test(size_t size)
{
    ThreadPool threadpool(std::string("TestThreadPool"));
    threadpool.setMaxQueueSize(size);
    threadpool.start(5);
    threadpool.run(print);
    threadpool.run(print);

    for (int i = 1; i <= 100; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "cycle task test: %d", i);
        threadpool.run(boost::bind(printString, std::string(buf)));
    }
    LOG_INFO << "done";
    usleep(1);
    struct timespec sp = {1, 0};
    nanosleep(&sp, NULL);
    LOG_INFO << "wake up from sleep";
}

int main(int argc, char const *argv[])
{
    test(5);
    LOG_INFO << "back from test()";
    return 0;
}
