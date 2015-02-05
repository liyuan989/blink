#include "AsyncLog.h"
#include "LogFile.h"
#include "Timestamp.h"
#include "TimeZone.h"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>

#include <time.h>
#include <assert.h>
#include <stdio.h>

namespace blink
{

TimeZone g_asyncLogTimeZone(3600 * 8, "UTC+8");

AsyncLog::AsyncLog(const string& basename,
                   size_t roll_size,
                   int flush_interval)
    : basename_(basename),
      roll_size_(roll_size),
      flush_interval_(flush_interval),
      running_(false),
      thread_(boost::bind(&AsyncLog::ThreadFunc, this), "AsyncLog"),
      latch_(1),
      mutex_(),
      cond_(mutex_),
      current_buffer_(new Buffer),
      next_buffer_(new Buffer),
      buffers_()
{
    current_buffer_->memset();
    next_buffer_->memset();
    buffers_.reserve(16);
}

void AsyncLog::append(const char* logline, size_t len)
{
    MutexLockGuard guard(mutex_);
    if (current_buffer_->availableSize() > len)
    {
        current_buffer_->append(logline, len);
    }
    else
    {
        buffers_.push_back(current_buffer_.release());
        if (next_buffer_)
        {
            current_buffer_ = boost::ptr_container::move(next_buffer_);  // move semantic
        }
        else
        {
            current_buffer_.reset(new Buffer);
        }
        current_buffer_->append(logline, len);
        cond_.wakeup();
    }
}

void AsyncLog::setTimeZone(const TimeZone& time_zone)
{
    g_asyncLogTimeZone = time_zone;
}

void AsyncLog::ThreadFunc()
{
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_, roll_size_, false);
    BufferPtr new_buffer1(new Buffer);
    BufferPtr new_buffer2(new Buffer);
    new_buffer1->memset();
    new_buffer2->memset();
    BufferVector buffers_to_write;
    buffers_to_write.reserve(16);
    while (running_)
    {
        assert(new_buffer1 && (new_buffer1->usedSize() == 0));
        assert(new_buffer2 && (new_buffer2->usedSize() == 0));
        assert(buffers_to_write.empty());
        {
            MutexLockGuard guard(mutex_);
            if (buffers_.empty())
            {
                cond_.timedWait(flush_interval_);
            }
            buffers_.push_back(current_buffer_.release());
            current_buffer_ = boost::ptr_container::move(new_buffer1);   // move semantic
            buffers_to_write.swap(buffers_);
            if (!next_buffer_)
            {
                next_buffer_ = boost::ptr_container::move(new_buffer2);  // move semantic
            }
        }
        assert(!buffers_to_write.empty());
        if (buffers_to_write.size() > 25)
        {
            char time_buf[32];
            formatTime(time_buf, sizeof(time_buf));
            char buf[256];
            snprintf(buf, sizeof(buf), "Dropped log message at %s, %zd larger buffers\n",
                     time_buf, buffers_to_write.size());
            fputs(buf, stderr);
            output.append(buf, strlen(buf));
            buffers_to_write.erase(buffers_to_write.begin() + 2, buffers_to_write.end());
        }
        for (size_t i = 0; i < buffers_to_write.size(); ++i)
        {
            output.append(buffers_to_write[i].data(), buffers_to_write[i].usedSize());
        }
        if (buffers_to_write.size() > 2)
        {
            buffers_to_write.resize(2);
        }
        if (!new_buffer1)
        {
            assert(!buffers_to_write.empty());
            new_buffer1 = buffers_to_write.pop_back();
            new_buffer1->reset();
        }
        if (!new_buffer2)
        {
            assert(!buffers_to_write.empty());
            new_buffer2 = buffers_to_write.pop_back();
            new_buffer2->reset();
        }
        buffers_to_write.clear();
        output.flush();
    }
    output.flush();
}

void AsyncLog::formatTime(char* time_buf, size_t len)
{
    int64_t microseconds_since_epoch = Timestamp::now().microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microseconds_since_epoch / Timestamp::kMicrosecondsPerSecond);
    int microseconds = static_cast<int>(microseconds_since_epoch % Timestamp::kMicrosecondsPerSecond);
    struct tm tm_time;
    if (g_asyncLogTimeZone.valid())
    {
        tm_time = g_asyncLogTimeZone.toLocalTime(seconds);
    }
    else
    {
        gmtime_r(&seconds, &tm_time);
    }
    if (g_asyncLogTimeZone.valid())
    {
        int n = snprintf(time_buf, len, "%4d%02d%02d %02d:%02d:%02d.%06d",
                         tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                         tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
        assert(n == 24);
        (void)n;
    }
    else
    {
        int n = snprintf(time_buf, len, "%4d%02d%02d %02d:%02d:%02d.%06dZ",
                         tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                         tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
        assert(n == 25);
        (void)n;
    }
}

}  // namespace blink
