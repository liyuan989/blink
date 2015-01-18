#include "Log.h"
#include "TimeZone.h"
#include "CurrentThread.h"

#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace blink
{

__thread char   t_error_buf[512];
__thread char   t_time[32];
__thread time_t t_lastsecond;

const char* strerror_rb(int saved_errno)
{
    return ::strerror_r(saved_errno, t_error_buf, sizeof(t_error_buf));
}

Log::LogLevel initLogLevel()
{
    if (::getenv("LOG_TRACE"))
    {
        return Log::TRACE;
    }
    else if (::getenv("LOG_DEBUG"))
    {
        return Log::DEBUG;
    }
    else
    {
        return Log::INFO;
    }
}

Log::LogLevel g_logLevel = initLogLevel();
const char*   kLogLevelName[Log::NUM_LOG_LEVELS] = {"TRACE ", "DEBUG ", "INFO  ", "WARN  ", "ERROR ", "FATAL "};
const int     kLogLevelNamelength = 6;

class LogStringHelper
{
public:
    LogStringHelper(const char* str, size_t len)
        : str_(str), len_(len)
    {
        assert(strlen(str) == len_);
    }

    const char*   str_;
    const size_t  len_;
};

inline LogStream& operator<<(LogStream& log_stream, LogStringHelper helper)
{
    log_stream.append(helper.str_, helper.len_);
    return log_stream;
}

inline LogStream& operator<<(LogStream& log_stream, const Log::SourceFile& file)
{
    log_stream.append(file.data_, file.size_);
    return log_stream;
}

void defaultOutput(const char* message, size_t len)
{
    size_t n = ::fwrite(message, STDOUT_FILENO, len, stdout);
    (void)n;
}

void defaultFlush()
{
    ::fflush(stdout);
}

Log::OutputFunc  g_output = defaultOutput;
Log::FlushFunc   g_flush = defaultFlush;
TimeZone         g_logTimeZone(3600 * 8, "UTC+8");   // default timezone:  UTC +8, time of Beijing(UTC+8)

Log::Impl::Impl(LogLevel level, int saved_errno, const SourceFile& file, int line)
    : time_(Timestamp::now()), stream_(), level_(level), line_(line), basename_(file)
{
    formatTime();
    tid();
    stream_ << LogStringHelper(tidString(), tidStringLength());
    stream_ << LogStringHelper(kLogLevelName[level], kLogLevelNamelength);
    if (saved_errno != 0)
    {
        stream_ << strerror_rb(saved_errno) << " (errno = " << saved_errno << ") ";
    }
}

//  struct tm
//  {
//      int tm_sec;      /* Seconds. [0-60] (1 leap second) */
//      int tm_min;      /* Minutes. [0-59] */
//      int tm_hour;     /* Hours.   [0-23] */
//      int tm_mday;     /* Day.     [1-31] */
//      int tm_mon;      /* Month.   [0-11] */
//      int tm_year;     /* Year.  [1900-current]  */
//      int tm_wday;     /* Day of week. [0-6] */
//      int tm_yday;     /* Days in year.[0-365] */
//      int tm_isdst;    /* DST.     [-1/0/1] */
//
//  # ifdef __USE_BSD
//      long  int tm_gmtoff;       /* Seconds east of UTC.  */
//      const char *tm_zone;       /* Timezone abbreviation.  */
//  # else
//      long  int __tm_gmtoff;     /* Seconds east of UTC.  */
//      const char *__tm_zone;     /* Timezone abbreviation.  */
//  # endif
//  };

void Log::Impl::formatTime()
{
    int64_t microseconds_since_epoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microseconds_since_epoch / Timestamp::kMicrosecondsPerSecond);
    int microseconds = static_cast<int>(microseconds_since_epoch % Timestamp::kMicrosecondsPerSecond);
    if (seconds != t_lastsecond)
    {
        t_lastsecond = seconds;
        struct tm tm_time;
        if (g_logTimeZone.valid())
        {
            tm_time = g_logTimeZone.toLocalTime(seconds);
        }
        else
        {
            gmtime_r(&seconds, &tm_time);
        }
        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d", tm_time.tm_year + 1900,
                           tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17);
        (void)len;
    }
    if (g_logTimeZone.valid())
    {
        Format fmt(".%06d ", microseconds);
        assert(fmt.length() == 8);
        stream_ << LogStringHelper(t_time, 17) << LogStringHelper(fmt.data(), 8);
    }
    else
    {
        Format fmt(".%06dZ ", microseconds);
        assert(fmt.length() == 9);
        stream_ << LogStringHelper(t_time, 17) << LogStringHelper(fmt.data(), 9);
    }
}

void Log::Impl::finish()
{
    stream_ << " - " << basename_ << ":" << line_ << '\n';
}

Log::Log(SourceFile file, int line)
    : impl_(INFO, 0, file, line)
{
}

Log::Log(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line)
{
}

Log::Log(SourceFile file, int line, LogLevel level, const char* func_name)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << func_name << ' ';
}

Log::Log(SourceFile file, int line, bool to_abort)
    : impl_((to_abort ? FATAL : ERROR), 0, file, line)
{
}

Log::~Log()
{
    impl_.finish();
    const LogStream::Buffer& buf(stream().buffer());
    g_output(buf.data(), buf.usedSize());
    if (impl_.level_ == FATAL)
    {
        g_flush();
        abort();
    }
}

void Log::setLogLevel(LogLevel level)
{
    g_logLevel = level;
}

void Log::setOutput(OutputFunc func)
{
    g_output = func;
}

void Log::setFlush(FlushFunc func)
{
    g_flush = func;
}

void Log::setTimeZone(const TimeZone& timezone)
{
    g_logTimeZone = timezone;
}

}  // namespace blink
