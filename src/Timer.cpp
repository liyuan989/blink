#include "Timer.h"

namespace blink
{

AtomicInt64 Timer::number_created_;

void Timer::run()
{
    callback_();
}

void Timer::restart(Timestamp now)
{
    if (repeat_)
    {
        expiration_ = addTime(now, interval_);
    }
    else
    {
        expiration_ = Timestamp::invalid();
    }
}

}  // namespace blink
