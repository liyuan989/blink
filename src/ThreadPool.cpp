#include "ThreadPool.h"
#include "Thread.h"
#include "Exception.h"

#include <boost/bind.hpp>

#include <exception>
#include <algorithm>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

namespace blink
{

ThreadPool::ThreadPool(const std::string& name)
    : mutex_(),
      not_empty_(mutex_),
      not_full_(mutex_),
      name_(name),
      max_queue_size_(0),
      running_(false)
{
}

ThreadPool::~ThreadPool()
{
    if (running_)
    {
        stop();
    }
}

void ThreadPool::start(int num_threads)
{
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i)
    {
        char buf_id[32];
        snprintf(buf_id, sizeof(buf_id), "%d", i + 1);
        threads_.push_back(new Thread(boost::bind(&ThreadPool::runInThread, this), name_ + buf_id));
        threads_[i].start();
    }
    if ((num_threads == 0) && thread_init_callback_)
    {
        thread_init_callback_();
    }
}

void ThreadPool::run(const Task& task)
{
    if (threads_.empty())
    {
        task();
    }
    else
    {
        MutexLockGuard guard(mutex_);
        while (isFull())
        {
            not_full_.wait();
        }
        assert(!isFull());
        queue_.push_back(task);
        not_empty_.wakeup();
    }
}

void ThreadPool::stop()
{
    {
    MutexLockGuard guard(mutex_);
    running_ = false;
    not_empty_.wakeupAll();
    }
    std::for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::join, _1));
}

bool ThreadPool::isFull() const
{
    assert(mutex_.isLockedByCurrentThread());
    return (max_queue_size_> 0) && (queue_.size() >= max_queue_size_);
}

void ThreadPool::runInThread()
{
    try
    {
        if (thread_init_callback_)
        {
            thread_init_callback_();
        }
        while (running_)
        {
            Task task(take());
            if (task)
            {
                task();
            }
        }
    }
    catch (const Exception& e)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", e.what());
        fprintf(stderr, "stack trace: %s\n", e.stackTrace());
        abort();
    }
    catch(const std::exception& e)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", e.what());
        abort();
    }
    catch (...)
    {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw;
    }
}

ThreadPool::Task ThreadPool::take()
{
    MutexLockGuard guard(mutex_);
    while (queue_.empty() && running_)
    {
        not_empty_.wait();
    }
    Task task;
    if (!queue_.empty())
    {
        task = queue_.front();
        queue_.pop_front();
        if (max_queue_size_ > 0)
        {
            not_full_.wakeup();
        }
    }
    return task;
}

}  // namespace blink
