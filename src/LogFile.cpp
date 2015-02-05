#include "LogFile.h"
#include "ProcessBase.h"
#include "MutexLock.h"
#include "FileTool.h"
#include "ProcessInfo.h"
#include "CurrentThread.h"
#include "TimeZone.h"
#include <boost/scoped_ptr.hpp>

#include <assert.h>
#include <time.h>
#include <stdio.h>

namespace blink
{

const int LogFile::kRollPerSeconds;
TimeZone g_logFileTimeZone(3600 * 8, "UTC+8");

LogFile::LogFile(const string& basename,
                 size_t roll_size,
                 bool thread_safe,
                 int flush_interval,
                 int check_every_n)
    : basename_(basename),
      roll_size_(roll_size),
      flush_interval_(flush_interval),
      check_every_n_(check_every_n),
      count_(0),
      mutex_(thread_safe ? new MutexLock : NULL),
      start_of_period_(0),
      last_roll_(0),
      last_flush_(0)
{
    assert(basename.find('/') == string::npos);
    rollFile();
}

LogFile::~LogFile()
{
}

void LogFile::append(const char* logline, size_t len)
{
    if (mutex_)
    {
        MutexLockGuard guard(*mutex_);
        appendUnlocked(logline, len);
    }
    else
    {
        appendUnlocked(logline, len);
    }
}

void LogFile::flush()
{
    if (mutex_)
    {
        MutexLockGuard guard(*mutex_);
        file_->flush();
    }
    else
    {
        file_->flush();
    }
}

bool LogFile::rollFile()
{
    time_t now = 0;
    string filename = getLogFileName(basename_, &now);
    time_t start = now / kRollPerSeconds * kRollPerSeconds;
    if (now > last_roll_)
    {
        last_roll_ = now;
        last_flush_ = now;
        start_of_period_ = start;
        file_.reset(new AppendFile(filename));
        return true;
    }
    return false;
}

void LogFile::setTimeZone(const TimeZone& time_zone)
{
    g_logFileTimeZone = time_zone;
}

void LogFile::appendUnlocked(const char* logline, size_t len)
{
    file_->appendFile(logline, len);
    if (file_->writtenBytes() > roll_size_)
    {
        rollFile();
    }
    else
    {
        ++count_;
        if (count_ > check_every_n_)
        {
            count_ = 0;
            time_t now = ::time(NULL);
            time_t this_period = now / kRollPerSeconds * kRollPerSeconds;
            if (this_period != start_of_period_)
            {
                rollFile();
            }
            else if ((now - last_flush_) > flush_interval_)
            {
                last_flush_ = now;
                flush();
            }
        }
    }
}

string LogFile::getLogFileName(const string& basename, time_t* now)
{
    string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;
    *now = ::time(NULL);
    struct tm tm_time = g_logFileTimeZone.toLocalTime(*now);
    //gmtime_r(now, &tm_time);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), ".%Y%m%d-%H%M%S.", &tm_time);
    filename += time_buf;
    filename += hostName();
    char pid_buf[32];
    snprintf(pid_buf, sizeof(pid_buf), ".%d", processes::getpid());
    filename += pid_buf;
    filename += ".log";
    return filename;
}

}  // namespace blink
