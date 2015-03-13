#ifndef __BLINK_DATE_H__
#define __BLINK_DATE_H__

#include <blink/Copyable.h>
#include <blink/Types.h>

#include <algorithm>
#include <time.h>

namespace blink
{

class Date
{
public:
    struct YearMonthDay
    {
        int year;
        int month;
        int day;
    };

    static const int kDaysPerWeek = 7;
    static const int kJuliandayOf1970_01_01;

    Date()
        : julian_day_number_(0)
    {
    }

    explicit Date(int julian_day_number)
        : julian_day_number_(julian_day_number)
    {
    }

    explicit Date(const struct tm& tm_time);
    Date(int year, int month, int day);

    string toString() const;
    YearMonthDay yearMonthDay() const;

    void swap(Date& rhs)
    {
        std::swap(julian_day_number_, rhs.julian_day_number_);
    }

    int year() const
    {
        return yearMonthDay().year;
    }

    int month() const
    {
        return yearMonthDay().month;
    }

    int day() const
    {
        return yearMonthDay().day;
    }

    int weekDay() const // [0 - 6] symbol of [Sunday - Saturday]
    {
        return (julian_day_number_ + 1) % kDaysPerWeek;
    }

    int julianDayNumber() const
    {
        return julian_day_number_;
    }

private:
    int  julian_day_number_;
};

inline bool operator==(Date lhs, Date rhs)
{
    return lhs.julianDayNumber() == rhs.julianDayNumber();
}

inline bool operator!=(Date lhs, Date rhs)
{
    return !(lhs == rhs);
}

inline bool operator<(Date lhs, Date rhs)
{
    return lhs.julianDayNumber() < rhs.julianDayNumber();
}

inline bool operator >=(Date lhs, Date rhs)
{
    return !(lhs < rhs);
}

inline bool operator>(Date lhs, Date rhs)
{
    return lhs.julianDayNumber() > rhs.julianDayNumber();
}

inline bool operator<=(Date lhs, Date rhs)
{
    return !(lhs > rhs);
}

}  // namespace blink

#endif
