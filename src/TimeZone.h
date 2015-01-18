#ifndef __BLINK_TIMEZONE_H__
#define __BLINK_TIMEZONE_H__

#include "Copyable.h"

#include <boost/shared_ptr.hpp>

#include <time.h>

namespace blink
{

class TimeZone : Copyable
{
public:
    TimeZone()
    {
    }

    explicit TimeZone(const char* zonefile);
    TimeZone(int east_of_utc, const char* timezone_name);

    struct tm toLocalTime(time_t seconds_since_epoch) const;
    time_t fromLocalTime(const struct tm& tm_time) const;

    static struct tm toUtcTime(time_t seconds_since_epoch, bool yday = false);
    static time_t fromUtcTime(const struct tm& tm_time);
    static time_t fromUtcTime(int year, int month, int day, int hour, int minute, int second); // year [1900-2500]

    bool valid() const
    {
        return static_cast<bool>(data_);
    }

    class Data;

private:
    boost::shared_ptr<Data> data_;
};

}  // namesapce blink

#endif
