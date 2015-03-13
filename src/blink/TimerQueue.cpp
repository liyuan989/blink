#include <blink/TimerQueue.h>
#include <blink/EventLoop.h>
#include <blink/Timer.h>
#include <blink/TimerId.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <sys/timerfd.h>
#include <unistd.h>

#include <iterator>
#include <assert.h>
#include <stdint.h>
#include <string.h>

namespace blink
{

int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        LOG_FATAL << "Failed in timerfd_create";
    }
    return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicrosecondsPerSecond);
    ts.tv_nsec = static_cast<long>(microseconds % Timestamp::kMicrosecondsPerSecond * 1000);
    return ts;
}

void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t how_many;
    int n = ::read(timerfd, &how_many, sizeof(how_many));
    LOG_TRACE << "TimerQueue::handleRead() " << how_many << " at " << now.toString();
    if (n != sizeof(how_many))
    {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}

// struct itimerspec
// {
//     struct timespec it_interval;  /* Interval for periodic timer */
//     struct timespec it_value;     /* Initial expiration */
// };

//  struct timespec
//  {
//      time_t tv_sec;     /* seconds */
//      long   tv_nsec;    /* nanoseconds */
//  };

void resetTimerfd(int timerfd, Timestamp expiration)
{
    struct itimerspec new_value;
    struct itimerspec old_value;
    memset(&new_value, 0, sizeof(new_value));
    memset(&old_value, 0, sizeof(old_value));
    new_value.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &new_value, &old_value);
    if (ret)
    {
        LOG_ERROR << "timerfd_settime";
    }
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfd_channel_(loop, timerfd_),
      timers_(),
      active_timers_(),
      calling_expired_timers_(false),
      canceling_timers_()
{
    timerfd_channel_.setReadCallback(boost::bind(&TimerQueue::handleRead, this));
    timerfd_channel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfd_channel_.disableAll();
    timerfd_channel_.remove();
    ::close(timerfd_);
    for (TimerList::iterator it = timers_.begin(); it != timers_.end(); ++it)
    {
        delete it->second;
    }
}

TimerId TimerQueue::addTimer(const TimerCallback& callback, Timestamp when, double interval)
{
    Timer* timer = new Timer(callback, when, interval);
    loop_->runInLoop(boost::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timer_id)
{
    loop_->runInLoop(boost::bind(&TimerQueue::cancelInLoop, this, timer_id));
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    loop_->assertInLoopThread();
    bool earliest_changed = insert(timer);
    if (earliest_changed)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::cancelInLoop(TimerId timer_id)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == active_timers_.size());
    ActiveTimer timer(timer_id.timer_, timer_id.sequence_);
    ActiveTimerSet::iterator it = active_timers_.find(timer);
    if (it != active_timers_.end())
    {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1);
        (void)n;
        delete it->first;
        active_timers_.erase(it);
    }
    else if (calling_expired_timers_)
    {
        canceling_timers_.insert(timer);
    }
    assert(timers_.size() == active_timers_.size());
}

void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);
    std::vector<Entry> expired = getExpired(now);
    calling_expired_timers_ = true;
    canceling_timers_.clear();
    for (std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it)
    {
        it->second->run();
    }
    calling_expired_timers_ = false;
    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    assert(timers_.size() == active_timers_.size());
    std::vector<Entry> expired;
#ifdef UINTPTR_MAX
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
#else
    Entry sentry(now, reinterpret_cast<Timer*>(static_cast<unsigned int>(-1)));
#endif
    TimerList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);
    std::copy(timers_.begin(), end, std::back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    for (std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it)
    {
        ActiveTimer timer(it->second, it->second->sequence());
        size_t n = active_timers_.erase(timer);
        assert(n == 1);
        (void)n;
    }
    assert(timers_.size() == active_timers_.size());
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp next_expire;
    for (std::vector<Entry>::const_iterator it = expired.begin(); it != expired.end(); ++it)
    {
        ActiveTimer timer(it->second, it->second->sequence());
        if (it->second->repeat() && canceling_timers_.find(timer) == canceling_timers_.end())
        {
            it->second->restart(now);
            insert(it->second);
        }
        else
        {
            delete it->second;
        }
    }
    if (!timers_.empty())
    {
        next_expire = timers_.begin()->second->expiration();
    }
    if (next_expire.valid())
    {
        resetTimerfd(timerfd_, next_expire);
    }
}

bool TimerQueue::insert(Timer* timer)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == active_timers_.size());
    bool earliest_changed = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    {
        earliest_changed = true;
    }
    {
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second);
        (void)result;
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result
            = active_timers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);
        (void)result;
    }
    assert(timers_.size() == active_timers_.size());
    return earliest_changed;
}

}  // namespace blink
