#include <blink/EventLoopThreadPool.h>
#include <blink/EventLoop.h>
#include <blink/EventLoopThread.h>

#include <assert.h>

#include <stdio.h>

namespace blink
{

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop, const string& name_arg)
    : base_loop_(base_loop),
      name_(name_arg),
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
        char buf[name_.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
        EventLoopThread* pthread = new EventLoopThread(cb, buf);
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
