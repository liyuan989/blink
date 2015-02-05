#include "BoundedBlockingQueue.h"
#include "Thread.h"
#include "CountDownLatch.h"
#include "CurrentThread.h"
#include "Timestamp.h"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>

#include <algorithm>
#include <map>
#include <stdio.h>

using namespace blink;

class BoundedBlockingQueueBench
{
public:
    BoundedBlockingQueueBench(size_t num)
        : task_queue_(num), latch_(num), threads_()
    {
        for (size_t i = 1; i <= num; ++i)
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "working thread %d", i);
            threads_.push_back(new Thread(boost::bind(&BoundedBlockingQueueBench::threadCallback, this),
                                          string(buf)));
            threads_.back().start();
        }
    }

    void run(size_t times)
    {
        printf("waiting for countdown latch...\n");
        latch_.wait();
        for (size_t i = 1; i <= times; ++i)
        {
            task_queue_.put(Timestamp::now());
        }
    }

    void joinAll()
    {
        for (size_t i = 0; i < threads_.size(); ++i)
        {
            task_queue_.put(Timestamp::invalid());
        }
        std::for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::join, _1));
    }

private:
    void threadCallback()
    {
        printf("thread id: %d  name: %s\n", tid(), threadName());
        latch_.countDown();
        std::map<int, int> delay_map;
        bool running = true;
        while (running)
        {
            Timestamp data = task_queue_.take();
            if (data.valid())
            {
                int delay = static_cast<int>(timeDifference(Timestamp::now(), data) * 1000 * 1000);
                ++delay_map[delay];
            }
            running = data.valid();
        }
        printf("%s stopped! tid: %d\n", threadName(), tid());
        for (std::map<int, int>::iterator it = delay_map.begin(); it != delay_map.end(); ++it)
        {
            printf("%s: tid = %d  delay = %d(usec)  count = %d\n", threadName(), tid(), it->first, it->second);
        }
    }

    BounededBlockingQueue<Timestamp>  task_queue_;
    CountDownLatch                    latch_;
    boost::ptr_vector<Thread>         threads_;
};


int main(int argc, char const *argv[])
{
    BoundedBlockingQueueBench bench(5);
    bench.run(10000);
    bench.joinAll();
    return 0;
}
