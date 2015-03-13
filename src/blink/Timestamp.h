#ifndef __BLINK_TIMESTAMP_H__
#define __BLINK_TIMESTAMP_H__

#include <blink/Copyable.h>
#include <blink/Types.h>

#include <algorithm>
#include <stdint.h>
#include <time.h>

namespace blink
{

class Timestamp : Copyable  // UTC time
{
public:
    Timestamp()
        : microseconds_since_epoch_(0)
    {
    }

    explicit Timestamp(int64_t microsecond_since_epoch)
        : microseconds_since_epoch_(microsecond_since_epoch)
    {
    }

    static Timestamp now();
    static Timestamp invalid();

    string toString() const;
    string toFormattedString(bool show_microseconds = true) const;

    void swap(Timestamp& rhs)
    {
        std::swap(microseconds_since_epoch_, rhs.microseconds_since_epoch_);
    }

    bool valid() const
    {
        return microseconds_since_epoch_ > 0;
    }

    int64_t microSecondsSinceEpoch() const
    {
        return microseconds_since_epoch_;
    }

    time_t secondsSinceEpoch() const
    {
        return static_cast<time_t>(microseconds_since_epoch_ / kMicrosecondsPerSecond);
    }

    static const int  kMicrosecondsPerSecond = 1000 * 1000;

private:
    int64_t  microseconds_since_epoch_;
};

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline bool operator!=(Timestamp lhs, Timestamp rhs)
{
    return !(lhs == rhs);
}

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator>=(Timestamp lhs, Timestamp rhs)
{
    return !(lhs < rhs);
}

inline bool operator>(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() > rhs.microSecondsSinceEpoch();
}

inline bool operator<=(Timestamp lhs, Timestamp rhs)
{
    return !(lhs > rhs);
}

inline double timeDifference(Timestamp high, Timestamp low)  // return seconds of diff.
{
    int64_t difference = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(difference) / Timestamp::kMicrosecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    int64_t microseconds = static_cast<int64_t>(seconds * Timestamp::kMicrosecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + microseconds);
}

}  // namespace blink

#endif
