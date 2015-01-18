#include "CountDownLatch.cpp"
#include "ThreadPool.h"
#include "Log.h"

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <string>
#include <stdio.h>

using namespace blink;

void print(const std::string& msg, int n, const boost::shared_ptr<CountDownLatch>& latch)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "I'm the task %d: %s", n, msg.c_str());
    LOG_INFO << buf;
    latch->countDown();
}

int main(int argc, char const *argv[])
{
    ThreadPool pool(std::string("CountDownLatchTest.ThreadPool"));
    pool.setMaxQueueSize(5);
    pool.start(5);
    boost::shared_ptr<CountDownLatch> latch(new CountDownLatch(90));
    for (int i = 1; i <= 100; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "(task %d)", i);
        pool.run(boost::bind(print, std::string(buf), i, latch));
    }
    latch->wait();
    LOG_INFO << "latch::getCount: " << latch->getCount();
    return 0;
}
