#ifndef __BLINK_THREADPOOL_H__
#define __BLINK_THREADPOOL_H__

#include <blink/Nocopyable.h>
#include <blink/MutexLock.h>
#include <blink/Condition.h>
#include <blink/Types.h>

#include <boost/function.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <deque>

namespace blink
{

class Thread;

class ThreadPool : Nocopyable
{
public:
    typedef boost::function<void ()> Task;

    explicit ThreadPool(const string& name_arg = string("ThreadPool"));
    ~ThreadPool();

    void start(int num_threads);
    void run(const Task& task);
    void stop();
    size_t queueSize() const;

    const string& name() const
    {
        return name_;
    }

    void setMaxQueueSize(size_t maxsize)
    {
        max_queue_size_ = maxsize;
    }

    void setThreadInitCallback(const Task& func)
    {
        thread_init_callback_ = func;
    }

private:
    bool isFull() const;
    void runInThread();
    Task take();

    mutable MutexLock          mutex_;
    Condition                  not_empty_;
    Condition                  not_full_;
    string                     name_;
    Task                       thread_init_callback_;
    boost::ptr_vector<Thread>  threads_;
    std::deque<Task>           queue_;
    size_t                     max_queue_size_;
    bool                       running_;
};

}  // namespace blink

#endif
