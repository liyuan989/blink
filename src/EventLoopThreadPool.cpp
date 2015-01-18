#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

#include <assert.h>

namespace blink
{

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop)
    : base_loop_(base_loop),
      started_(false),
      number_threads_(0),
      next_(0),
      threads_(),
      loops_()
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    assert(!started_);
    base_loop_->assertInLoopThread();
    started_ = true;
    for (int i = 0; i < number_threads_; ++i)
    {
        EventLoopThread* pthread = new EventLoopThread(cb);
        threads_.push_back(pthread);
        loops_.push_back(pthread->startLoop());
    }
    if (number_threads_ == 0 && cb)
    {
        cb(base_loop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    base_loop_->assertInLoopThread();
    assert(started_);
    EventLoop* loop = base_loop_;
    if (!loops_.empty())
    {
        loop = loops_[next_];
        ++next_;
        if (static_cast<size_t>(next_) >= loops_.size())
        {
            next_ = 0;
        }
    }
    return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hash_code)
{
    base_loop_->assertInLoopThread();
    EventLoop* loop = base_loop_;
    if (!loops_.empty())
    {
        loop = loops_[hash_code % loops_.size()];
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    base_loop_->assertInLoopThread();
    assert(started_);
    if (loops_.empty())
    {
        return std::vector<EventLoop*>(1, base_loop_);
    }
    else
    {
        return loops_;
    }
}

}  // namespace blink
