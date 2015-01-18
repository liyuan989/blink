#ifndef __BLINK_LOGFILE_H__
#define __BLINK_LOGFILE_H__

#include "Nocopyable.h"

#include <boost/scoped_ptr.hpp>

#include <string>

namespace blink
{

class MutexLock;
class AppendFile;
class TimeZone;

class LogFile : Nocopyable
{
public:
    LogFile(const std::string& basename,
            size_t roll_size,
            bool thread_safe = true,
            int flush_interval = 3,
            int check_every_n = 1024);

    ~LogFile();

    void append(const char* logline, size_t len);
    void flush();
    bool rollFile();

    static void setTimeZone(const TimeZone& time_zone);

private:
    void appendUnlocked(const char* logline, size_t len);
    static std::string getLogFileName(const std::string& basename, time_t* now);

    const std::string              basename_;
    const size_t                   roll_size_;
    const int                      flush_interval_;
    const int                      check_every_n_;
    int                            count_;
    boost::scoped_ptr<MutexLock>   mutex_;
    time_t                         start_of_period_;
    time_t                         last_roll_;
    time_t                         last_flush_;
    boost::scoped_ptr<AppendFile>  file_;

    const static int kRollPerSeconds = 60 * 60 * 24;
};

}  // namespace blink

#endif
