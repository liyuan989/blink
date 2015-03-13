#ifndef __BLINK_TIMERID_H__
#define __BLINK_TIMERID_H__

#include <blink/Copyable.h>

#include <stddef.h>

namespace blink
{

class Timer;

class TimerId : Copyable
{
public:
    TimerId()
        : timer_(NULL), sequence_(0)
    {
    }

    TimerId(Timer* timer, int64_t sequence)
        : timer_(timer), sequence_(sequence)
    {
    }

    friend class TimerQueue;

private:
    Timer*   timer_;
    int64_t  sequence_;
};

}  // namespace blink

#endif
