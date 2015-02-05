#include "Date.h"

#include <boost/static_assert.hpp>

#include <time.h>
#include <stdio.h>

namespace blink
{

BOOST_STATIC_ASSERT(sizeof(int) >= sizeof(int32_t));

int getJulianDayNumber(int year, int month, int day)
{
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12*a - 3;
    return day + (153*m + 2) / 5 + y*365 + y/4 - y/100 + y/400 - 32045;
}

Date::YearMonthDay getYearMonthDay(int julian_day_number)
{
    int a = julian_day_number + 32044;
    int b = (4*a + 3) / 146097;
    int c = a - ((b * 146097) / 4);
    int d = (4*c + 3) / 1461;
    int e = c - ((1461 * d) / 4);
    int m = (5*e + 2) / 153;
    Date::YearMonthDay result;
    result.year = b * 100 + d - 4800  + (m / 10);
    result.month = m + 3 - 12 * (m / 10);
    result.day = e - ((153*m + 2) / 5) + 1;
    return result;
}

const int Date::kDaysPerWeek;
const int Date::kJuliandayOf1970_01_01 = getJulianDayNumber(1970, 1, 1);

Date::Date(const struct tm& tm_time)
    : julian_day_number_(getJulianDayNumber(tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday))
{
}

Date::Date(int years, int months, int days)
    : julian_day_number_(getJulianDayNumber(years, months, days))
{
}

string Date::toString() const
{
    char buf[32];
    YearMonthDay ymd(yearMonthDay());
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", ymd.year, ymd.month, ymd.day);
    return buf;
}

Date::YearMonthDay Date::yearMonthDay() const
{
    return getYearMonthDay(julian_day_number_);
}

}  // namespace blink
