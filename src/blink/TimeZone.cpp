#include <blink/TimeZone.h>
#include <blink/Date.h>
#include <blink/Nocopyable.h>
#include <blink/Endian.h>

#include <boost/shared_ptr.hpp>

#include <sys/time.h>

#include <algorithm>
#include <stdexcept>
#include <vector>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

namespace blink
{

const int kSecondsPerDay = 60 * 60 * 24;

namespace detail
{

struct Transition
{
    time_t  gmt_time;
    time_t  local_time;
    int     local_time_index;

    Transition(time_t gmt, time_t local, int index)
        : gmt_time(gmt), local_time(local), local_time_index(index)
    {
    }
};

struct Comp
{
    bool compare_gmt;

    Comp(bool gmt)
        : compare_gmt(gmt)
    {
    }

    bool operator()(const Transition& lhs, const Transition& rhs) const
    {
        if (compare_gmt)
        {
            return lhs.gmt_time < rhs.gmt_time;
        }
        else
        {
            return lhs.local_time < rhs.local_time;
        }
    }

    bool equal(const Transition& lhs, const Transition& rhs) const
    {
        if (compare_gmt)
        {
            return lhs.gmt_time == rhs.gmt_time;
        }
        else
        {
            return lhs.local_time == rhs.local_time;
        }
    }
};

struct LocalTime
{
    time_t  gmt_offset;
    bool    is_dst;
    int     arrb_index;

    LocalTime(time_t offset, bool dst, int index)
        : gmt_offset(offset), is_dst(dst), arrb_index(index)
    {
    }
};

class File : Nocopyable
{
public:
    File(const char* filename)
        : fp_(::fopen(filename, "rb"))
    {
    }

    ~File()
    {
        if (fp_)
        {
            ::fclose(fp_);
        }
    }

    bool valid() const
    {
        return fp_;
    }

    string readBytes(size_t n)
    {
        char buf[n];
        ssize_t nread = ::fread(buf, 1, n, fp_);
        if (static_cast<size_t>(nread) != n)
        {
            throw std::logic_error("no enough data");
        }
        return string(buf, n);
    }

    int32_t readInt32()
    {
        int32_t val = 0;
        ssize_t nread = ::fread(&val, 1, sizeof(int32_t), fp_);
        if (nread != sizeof(int32_t))
        {
            throw std::logic_error("bad int32_t data");
        }
        return sockets::networkToHost32(val);
    }

    uint8_t readUint8()
    {
        uint8_t val = 0;
        ssize_t nread = ::fread(&val, 1, sizeof(uint8_t), fp_);
        if (nread != sizeof(uint8_t))
        {
            throw std::logic_error("bad uint8_t data");
        }
        return val;
    }

private:
    FILE*  fp_;
};

}  // namespace detail

struct TimeZone::Data
{
    std::vector<detail::Transition>   transition;
    std::vector<detail::LocalTime>    local_time;
    std::vector<string>               name;
    string                            abbreviation;
};

namespace detail
{

bool readTimeZoneFile(const char* zone_file, TimeZone::Data* data)
{
    File f(zone_file);
    if (f.valid())
    {
        try
        {
            string head = f.readBytes(4);
            if (head != "TZif")
            {
                throw std::logic_error("bad head");
            }
            string version = f.readBytes(1);
            f.readBytes(15);
            int32_t is_gmt_count = f.readInt32();
            int32_t is_std_count = f.readInt32();
            int32_t leap_count = f.readInt32();
            int32_t time_count = f.readInt32();
            int32_t type_count = f.readInt32();
            int32_t char_count = f.readInt32();

            std::vector<int32_t> trans;
            std::vector<int> local_time;
            trans.reserve(time_count);
            for (int i = 0; i < time_count; ++i)
            {
                trans.push_back(f.readInt32());
            }
            for (int i = 0; i < time_count; ++i)
            {
                uint8_t local = f.readUint8();
                local_time.push_back(local);
            }
            for (int i = 0; i < type_count; ++i)
            {
                int32_t gmt_offset = f.readInt32();
                uint8_t is_dst = f.readUint8();
                uint8_t abbrind = f.readUint8();
                data->local_time.push_back(LocalTime(gmt_offset, is_dst, abbrind));
            }
            for (int i = 0; i < time_count; ++i)
            {
                int local_index = local_time[i];
                time_t localtime = trans[i] + data->local_time[local_index].gmt_offset;
                data->transition.push_back(Transition(trans[i], localtime, local_index));
            }
            data->abbreviation = f.readBytes(char_count);
            //for (int i = 0; i < leap_count; ++i)
            //{
            //  int32_t leap_time = f.readInt32();
            //  int32_t cum_leap = f.readInt32();
            //}
            (void)leap_count;
            (void)is_std_count;
            (void)is_gmt_count;
        }
        catch (std::logic_error& e)
        {
            fprintf(stderr, "%s\n", e.what());
        }
    }
    return true;
}

const LocalTime* findLocalTime(const TimeZone::Data& data, Transition sentry, Comp comp)
{
    const LocalTime* result = NULL;
    if (data.transition.empty() || comp(sentry, data.transition.front()))
    {
        result = &data.local_time.front();
    }
    else
    {
        std::vector<Transition>::const_iterator iter = std::lower_bound(data.transition.begin(),
                                                                  data.transition.end(),
                                                                  sentry,
                                                                  comp);
        if (iter != data.transition.end())
        {
            if (!comp.equal(*iter, sentry))
            {
                assert(iter != data.transition.begin());
                --iter;
            }
            result = &data.local_time[iter->local_time_index];
        }
        else
        {
            result = &data.local_time[data.transition.back().local_time_index];
        }
    }
    return result;
}

}  // namespace detail

TimeZone::TimeZone(const char* zonefile)
    : data_(new TimeZone::Data)
{
    if (!detail::readTimeZoneFile(zonefile, data_.get()))
    {
        data_.reset();
    }
}

TimeZone::TimeZone(int east_of_utc, const char* timezone_name)
    : data_(new TimeZone::Data)
{
    data_->local_time.push_back(detail::LocalTime(east_of_utc, false, 0));
    data_->abbreviation = timezone_name;
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

struct tm TimeZone::toLocalTime(time_t seconds_since_epoch) const
{
    struct tm result;
    memset(&result, 0, sizeof(result));
    assert(data_ != NULL);
    const Data& data(*data_);
    detail::Transition sentry(seconds_since_epoch, 0, 0);
    const detail::LocalTime* local = detail::findLocalTime(data, sentry, detail::Comp(true));

    if (local)
    {
        time_t local_seconds = seconds_since_epoch + local->gmt_offset;
        ::gmtime_r(&local_seconds, &result);
        result.tm_isdst = local->is_dst;
        result.tm_gmtoff = local->gmt_offset;
        result.tm_zone = &data.abbreviation[local->arrb_index];
    }
    return result;
}

time_t TimeZone::fromLocalTime(const struct tm& tm_time) const
{
    assert(data_ != NULL);
    const Data& data(*data_);
    struct tm temp = tm_time;
    time_t seconds = ::timegm(&temp);
    detail::Transition sentry(0, seconds, 0);
    const detail::LocalTime* local = detail::findLocalTime(data, sentry, detail::Comp(false));

    if (tm_time.tm_isdst)
    {
        struct tm try_tm = toLocalTime(seconds - local->gmt_offset);
        if (!try_tm.tm_isdst && (try_tm.tm_hour == tm_time.tm_hour) && (try_tm.tm_min == tm_time.tm_min))
        {
            seconds -= 3600;
        }
    }
    return seconds - local->gmt_offset;
}

inline void fillHMS(unsigned seconds, struct tm* utc)
{
    utc->tm_sec = seconds % 60;
    unsigned minutes = seconds / 60;
    utc->tm_min = minutes % 60;
    utc->tm_hour = minutes / 60;
}

struct tm TimeZone::toUtcTime(time_t seconds_since_epoch, bool yday)
{
    struct tm utc;
    memset(&utc, 0, sizeof(utc));
    utc.tm_zone = "GMT";
    int seconds = static_cast<int>(seconds_since_epoch % kSecondsPerDay);
    int days = static_cast<int>(seconds_since_epoch / kSecondsPerDay);
    if (seconds < 0)
    {
        seconds += kSecondsPerDay;
        --days;
    }
    fillHMS(seconds, &utc);
    Date date(days + Date::kJuliandayOf1970_01_01);
    Date::YearMonthDay ymd = date.yearMonthDay();
    utc.tm_year = ymd.year - 1900;
    utc.tm_mon = ymd.month - 1;
    utc.tm_mday = ymd.day;
    utc.tm_wday = date.weekDay();
    if (yday)
    {
        Date start_of_year(ymd.year, 1, 1);
        utc.tm_yday = date.julianDayNumber() - start_of_year.julianDayNumber();
    }
    return utc;
}

time_t TimeZone::fromUtcTime(const struct tm& tm_time)
{
    return fromUtcTime(tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                       tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
}

time_t TimeZone::fromUtcTime(int year, int month, int day, int hour, int minute, int second)
{
    Date date(year, month, day);
    int seconds_of_day = hour * 3600 + minute * 60 + second;
    time_t days = date.julianDayNumber() - Date::kJuliandayOf1970_01_01;
    return days * kSecondsPerDay + seconds_of_day;
}

}  // namespace blink
