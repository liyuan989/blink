#ifndef __BLINK_TIMER_H__
#define __BLINK_TIMER_H__

#include <blink/Nocopyable.h>
#include <blink/Timestamp.h>
#include <blink/Atomic.h>
#include <blink/Callbacks.h>

namespace blink
{

class Timer : Nocopyable
{
public:
    Timer(const TimerCallback callback, Timestamp when, double interval)
        : callback_(callback),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(number_created_.incrementAndGet())
    {
    }

    void run();
    void restart(Timestamp now);

    Timestamp expiration() const
    {
        return expiration_;
    }

    bool repeat() const
    {
        return repeat_;
    }

    int64_t sequence() const
    {
        return sequence_;
    }

    static int64_t numberCreated()
    {
        return number_created_.get();
    }

private:
    const TimerCallback  callback_;
    Timestamp            expiration_;
    const double         interval_;
    const bool           repeat_;
    const int64_t        sequence_;

    static AtomicInt64   number_created_;
};

}  // namespace blink

#endif
