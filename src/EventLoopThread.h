#ifndef __BLINK_EVENTLOOPTHREAD_H__
#define __BLINK_EVENTLOOPTHREAD_H__

#include "Nocopyable.h"
#include "MutexLock.h"
#include "Condition.h"
#include "Thread.h"

#include <boost/function.hpp>

namespace blink
{

class EventLoop;

class EventLoopThread : Nocopyable
{
public:
    typedef boost::function<void (EventLoop*)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                    const string& name = string());
    ~EventLoopThread();

    EventLoop* startLoop();

private:
    void ThreadFunc();

    EventLoop*          loop_;
    bool                exiting_;
    Thread              thread_;
    MutexLock           mutex_;
    Condition           cond_;
    ThreadInitCallback  callback_;

};

}  // namespace blink

#endif
