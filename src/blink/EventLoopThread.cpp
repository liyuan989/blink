#include <blink/EventLoopThread.h>
#include <blink/EventLoop.h>

#include <boost/bind.hpp>

#include <assert.h>

namespace blink
{

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const string& name)
    : loop_(NULL),
      exiting_(false),
      thread_(boost::bind(&EventLoopThread::ThreadFunc, this), name),
      mutex_(),
      cond_(mutex_),
      callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != NULL)     // not 100% race-free, eg. threadFunc could be running callback_.
    {
        loop_->quit();
        thread_.join();    // still a tiny chance to call destructor object, if threadFunc exit just now.
    }                      // but when EventLoopthread destructs, usually programming is exiting anyway.
}

EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    thread_.start();
    {
        MutexLockGuard guard(mutex_);
        while (loop_ == NULL)
        {
            cond_.wait();
        }
    }
    return loop_;
}

void EventLoopThread::ThreadFunc()
{
    EventLoop loop;
    if (callback_)
    {
        callback_(&loop);
    }
    {
        MutexLockGuard guard(mutex_);
        loop_ = &loop;
        cond_.wakeup();
    }
    loop.loop();
    //assert(exiting_);
    loop_ = NULL;
}

}  // namespace blink
