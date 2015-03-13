#ifndef __BLINK_EVENTLOOP_H__
#define __BLINK_EVENTLOOP_H__

#include <blink/Nocopyable.h>
#include <blink/MutexLock.h>
#include <blink/Timestamp.h>
#include <blink/Callbacks.h>
#include <blink/TimerId.h>
#include <blink/CurrentThread.h>

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/any.hpp>

#include <sys/types.h>

#include <vector>
#include <stdint.h>

namespace blink
{

class Channel;
class TimerQueue;
class Poller;

// Reactor, at most one per thread.
class EventLoop : Nocopyable
{
public:
    typedef boost::function<void ()> Functor;

    EventLoop();
    ~EventLoop();

    // Loops forever.
    // Must be called in the same thread as creation of the object.
    void loop();

    // Quits loop.
    // This is not 100% thread safe, if you call through a raw pointer,
    // better to call through shared_ptr<EventLoop> for 100% safety.
    void quit();

    // Runs callback immediately in the loop thread.
    // It wakes up the loop, and run the "cb".
    // If in the same loop thread, "cb" is run within the function.
    // Safe to call from other threads.
    void runInLoop(const Functor& cb);

    // Queues callback in the loop thread.
    // Runs after finish pooling.
    // Safe to call from other threads.
    void queueInLoop(const Functor& cb);

    // Runs callback at "time".
    // Safe to call from other threads.
    TimerId runAt(const Timestamp& time, const TimerCallback& cb);

    // Runs callback after "delay" seconds.
    // Safe to call from other threads.
    TimerId runAfter(double delay, const TimerCallback& cb);

    // Runs callback every "interval" seconds.
    // Safe to call from other threads.
    TimerId runEvery(double interval, const TimerCallback& cb);

    // Cancel the timer.
    // Safe to call from other threads.
    void cancel(TimerId timer_id);

    void wakeup();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    static EventLoop* getEventLoopOfCurrentThread();

    // Time when poll returns, usuanlly means data arrival.
    Timestamp pollReturnTime() const
    {
        return poll_return_time_;
    }

    int64_t iteration() const
    {
        return iteration_;
    }

    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }

    bool isInLoopThread() const
    {
        return thread_id_ == blink::tid();
    }

    bool eventHandling() const
    {
        return event_handling_;
    }

    void setContext(const boost::any& context)
    {
        context_ = context;
    }

    const boost::any& getContext() const
    {
        return context_;
    }

    boost::any* getMutableContext()
    {
        return &context_;
    }

private:
    void abortNotInLoopThread();
    void handleRead();                // wake up
    void doPendingFunctors();
    void printActiveChannels() const; // DEBUG

    typedef std::vector<Channel*>  ChannelList;

    bool                           looping_;
    bool                           quit_;
    bool                           event_handling_;
    bool                           calling_pending_functors_;
    int64_t                        iteration_;
    const pid_t                    thread_id_;
    Timestamp                      poll_return_time_;
    boost::scoped_ptr<Poller>      poller_;
    boost::scoped_ptr<TimerQueue>  timer_queue_;
    int                            wakeup_fd_;
    boost::scoped_ptr<Channel>     wakeup_channel_;
    boost::any                     context_;
    ChannelList                    active_channels_;             // scratch variable
    Channel*                       current_active_channel_;      // scratch variable
    MutexLock                      mutex_;
    std::vector<Functor>           pending_functors_;            // guarded by mutex_
};

}  //namespace blink

#endif
