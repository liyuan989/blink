#ifndef __BLINK_LOG_H__
#define __BLINK_LOG_H__

#include <blink/Nocopyable.h>
#include <blink/LogStream.h>
#include <blink/Timestamp.h>

#include <string.h>

namespace blink
{

const char* strerror_tl(int saved_errno);

class TimeZone;

class Log : Nocopyable
{
public:
    typedef void (*OutputFunc)(const char*, size_t);
    typedef void (*FlushFunc)();

    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };

    class SourceFile
    {
    public:
        template<int N>
        SourceFile(const char (&array)[N])
            : data_(array), size_(N - 1)
        {
            const char* slash = strrchr(data_, '/');
            if (slash)
            {
                data_ = slash + 1;
                size_ -= static_cast<int>(data_ - array);
            }
        }

        explicit SourceFile(const char* filename)
            : data_(filename)
        {
            const char* slash = strrchr(data_, '/');
            if (slash)
            {
                data_ = slash + 1;
            }
            size_ = static_cast<int>(strlen(data_));
        }

        const char*  data_;
        int          size_;
    };

    Log(SourceFile file, int line);
    Log(SourceFile file, int line, LogLevel level);
    Log(SourceFile file, int line, LogLevel level, const char* func_name);
    Log(SourceFile file, int line, bool to_abort);
    ~Log();

    static LogLevel logLevel();
    static void setLogLevel(LogLevel level);
    static void setOutput(OutputFunc func);
    static void setFlush(FlushFunc func);
    static void setTimeZone(const TimeZone& timezone);

    LogStream& stream()
    {
        return impl_.stream_;
    }

private:
    class Impl
    {
    public:
        Impl(LogLevel level, int saved_errno, const SourceFile& file, int line);

        void formatTime();
        void finish();

    Timestamp   time_;
    LogStream   stream_;
    LogLevel    level_;
    int         line_;
    SourceFile  basename_;
    };

    Impl  impl_;
};

extern Log::LogLevel g_logLevel;

inline Log::LogLevel Log::logLevel()
{
    return g_logLevel;
}

template<typename T>
T* checkNotNull(Log::SourceFile file, int line, const char* message, T* p)
{
    if (p == NULL)
    {
        Log(file, line, Log::FATAL).stream() << message;
    }
    return p;
}

#define LOG_TRACE \
if (blink::Log::logLevel() <= blink::Log::TRACE) \
    blink::Log(__FILE__, __LINE__, blink::Log::TRACE, __func__).stream()

#define LOG_DEBUG \
if (blink::Log::logLevel() <= blink::Log::DEBUG) \
    blink::Log(__FILE__, __LINE__, blink::Log::DEBUG, __func__).stream()

#define LOG_INFO \
if (blink::Log::logLevel() <= blink::Log::INFO) \
    blink::Log(__FILE__, __LINE__).stream()

#define LOG_WARN   blink::Log(__FILE__, __LINE__, blink::Log::WARN).stream()
#define LOG_ERROR  blink::Log(__FILE__, __LINE__, false).stream()
#define LOG_FATAL  blink::Log(__FILE__, __LINE__, true).stream()

#define CHECK_NOTNULL(ptr) \
    ::blink::checkNotNull(__FILE__, __LINE__, "'" #ptr "' Mustn't be NULL", (ptr))

}  // namespace blink

#endif
