#ifndef __BLINK_ASYNCLOG_H__
#define __BLINK_ASYNCLOG_H__

#include <blink/Nocopyable.h>
#include <blink/LogStream.h>
#include <blink/BoundedBlockingQueue.h>
#include <blink/BlockingQueue.h>
#include <blink/CountDownLatch.h>
#include <blink/Thread.h>
#include <blink/MutexLock.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>

namespace blink
{

class TimeZone;

class AsyncLog : Nocopyable
{
    typedef detail::FixedBuffer<LogStream::kLargeBuffer>  Buffer;
    typedef boost::ptr_vector<Buffer>                     BufferVector;
    typedef boost::ptr_vector<Buffer>::auto_type          BufferPtr;

public:
    AsyncLog(const string& basename, size_t roll_size, int flush_interval = 3);

    ~AsyncLog()
    {
        if (running_)
        {
            stop();
        }
    }

    void append(const char* logline, size_t len);

    void start()
    {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop()
    {
        running_ = false;
        cond_.wakeup();
        thread_.join();
    }

    static void setTimeZone(const TimeZone& time_zone);

private:
    AsyncLog(const AsyncLog&);
    AsyncLog& operator=(const AsyncLog&);

    void ThreadFunc();
    void formatTime(char* time_buf, size_t len);

    string          basename_;
    size_t          roll_size_;
    const int       flush_interval_;
    bool            running_;
    Thread          thread_;
    CountDownLatch  latch_;
    MutexLock       mutex_;
    Condition       cond_;
    BufferPtr       current_buffer_;
    BufferPtr       next_buffer_;
    BufferVector    buffers_;
};

}  // namespace blink

#endif
