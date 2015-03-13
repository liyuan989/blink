#include <blink/AsyncLog.h>
#include <blink/Log.h>
#include <blink/Thread.h>
#include <blink/CurrentThread.h>
#include <blink/Timestamp.h>

#include <boost/scoped_ptr.hpp>

#include <sys/resource.h>
#include <sys/time.h>

#include <string>
#include <time.h>
#include <stdio.h>

using namespace blink;

const int kRollSize = 64 * 1024;

boost::scoped_ptr<AsyncLog> async_log;

void output(const char* data, size_t len)
{
    async_log->append(data, len);
}

void func()
{
    char buf[64];
    for (int i = 1; i <= 1000; ++i)
    {
        snprintf(buf, sizeof(buf), "%s: tid = %d, the %d print", threadName(), tid(), i);
        LOG_INFO << buf;
    }
}

void bench(bool longlog)
{
    const int kBatch = 1000;
    int count = 0;
    string longstr(3000, 'X');
    for (int i = 0; i < 32; ++i)
    {
        Timestamp start = Timestamp::now();
        for (int j = 0; j < kBatch; ++j)
        {
            LOG_INFO << "hey boy! 9876543210" << "QWERASDFZXC"
                     << " poilkujhuhhd " << (longlog ? longstr : " ") << count;
            ++count;
        }
        Timestamp end = Timestamp::now();
        printf("%g\n", timeDifference(end, start) * 1000000 / kBatch); // (picosecond, 1ps = 1000000ns);
        struct timespec spec = {0, 500*1000*1000};
        nanosleep(&spec, NULL);
    }
}

int main(int argc, char const *argv[])
{
    {
        size_t kBytesPerGB = 1000 * 1024 * 1024;
        rlimit rl = {2 * kBytesPerGB, 2 * kBytesPerGB};
        setrlimit(RLIMIT_AS, &rl);
    }
    Log::setOutput(output);
    char buf[64];
    snprintf(buf, sizeof(buf), "%s", argv[0]);
    async_log.reset(new AsyncLog(::basename(buf), kRollSize));
    Thread thread1(func);
    Thread thread2(func);
    Thread thread3(func);
    async_log->start();
    thread1.start();
    thread2.start();
    thread3.start();
    thread1.join();
    thread2.join();
    thread3.join();

    printf("main thread: %d\n", tid());
    bench(argc > 1 ? true : false);
    async_log->stop();
    printf("main end!\n");
    return 0;
}
