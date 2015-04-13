#include <blink/CurrentThread.h>
#include <blink/ThreadBase.h>
#include <blink/ProcessBase.h>

#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>

#include <sys/types.h>

#include <stdint.h>
#include <time.h>
#include <stdio.h>

namespace blink
{

namespace current_thread
{

__thread int t_cache_tid = 0;
__thread char t_tid_string[32];
__thread int t_tid_string_length = 6;
__thread const char* t_thread_name = "unknown";

const bool is_same_type = boost::is_same<int, pid_t>::value;
BOOST_STATIC_ASSERT(is_same_type);

void cacheTid()
{
    if (t_cache_tid == 0)
    {
        t_cache_tid = threads::gettid();
        t_tid_string_length = snprintf(t_tid_string, sizeof(t_tid_string), "%5d ", t_cache_tid);
    }
}

bool isMainThread()
{
    return tid() == processes::getpid();
}

//  struct timespec
//  {
//      time_t tv_sec;  /* seconds */
//      long tv_nsec;   /* nanoseconds */
//  };

void sleepMicroseconds(int64_t microseconds)
{
    struct timespec spec;
    spec.tv_sec = static_cast<time_t>(microseconds / (1000 * 1000));
    spec.tv_nsec = static_cast<long>(microseconds % (1000 * 1000) * 1000);
    ::nanosleep(&spec, NULL);
}

}  // namespace current_thread

}  // namespace blink

