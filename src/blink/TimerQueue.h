#ifndef __BLINK_TIMERQUEUE_H__
#define __BLINK_TIMERQUEUE_H__

#include <blink/Nocopyable.h>
#include <blink/MutexLock.h>
#include <blink/Timestamp.h>
#include <blink/Callbacks.h>
#include <blink/Channel.h>

#include <set>
#include <vector>

namespace blink
{

class EventLoop;
class Timer;
class TimerId;

class TimerQueue : Nocopyable
{
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId addTimer(const TimerCallback& callback, Timestamp when, double interval);
    void cancel(TimerId timer_id);

private:
    // may use unique_ptr<Timer> instead of raw pointer in C++11.
    typedef std::pair<Timestamp, Timer*>  Entry;
    typedef std::set<Entry>               TimerList;
    typedef std::pair<Timer*, int64_t>    ActiveTimer;
    typedef std::set<ActiveTimer>         ActiveTimerSet;

    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timer_id);

    // called when timerfd alarms.
    void handleRead();

    // move out all expired timers.
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);
    bool insert(Timer* timer);

    EventLoop*      loop_;
    const int       timerfd_;
    Channel         timerfd_channel_;
    TimerList       timers_;                     // sorted by expiration time.
    ActiveTimerSet  active_timers_;              // sorted by Timer's address.
    bool            calling_expired_timers_;     // atomic.
    ActiveTimerSet  canceling_timers_;           // for cancel().
};

}  // namespace blink

#endif
