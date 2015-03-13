#include <blink/Timestamp.h>

#include <boost/static_assert.hpp>

#include <sys/time.h>

#include <inttypes.h>
#include <stdio.h>

namespace blink
{

BOOST_STATIC_ASSERT(sizeof(Timestamp) == sizeof(int64_t));

const int  Timestamp::kMicrosecondsPerSecond;

//  struct timeval
//  {
//      time_t       tv_sec;    /* seconds */
//      suseconds_t  tv_usec;   /* microseconds */
//  };

Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = static_cast<int64_t>(tv.tv_sec);
    return Timestamp(seconds * kMicrosecondsPerSecond + tv.tv_usec);
}

Timestamp Timestamp::invalid()
{
    return Timestamp();
}

string Timestamp::toString() const
{
    char buf[32] = {0};
    int64_t seconds = microseconds_since_epoch_ / kMicrosecondsPerSecond;
    int64_t microseconds = microseconds_since_epoch_ % kMicrosecondsPerSecond;
    snprintf(buf, sizeof(buf) - 1, "%" PRId64".06%" PRId64"", seconds, microseconds);
    return buf;
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

string Timestamp::toFormattedString(bool show_microseconds) const
{
    char buf[32] = {0};
    time_t seconds = static_cast<time_t>(microseconds_since_epoch_ / kMicrosecondsPerSecond);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);
    if (show_microseconds)
    {
        int mocro_seconds = static_cast<int>(microseconds_since_epoch_ % kMicrosecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d", tm_time.tm_year + 1900, tm_time.tm_mon + 1,
                 tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, mocro_seconds);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d", tm_time.tm_year + 1900, tm_time.tm_mon + 1,
                 tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
}

}
