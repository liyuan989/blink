#ifndef __BLINK_BLOCKINGQUEUE_H__
#define __BLINK_BLOCKINGQUEUE_H__

#include "Nocopyable.h"
#include "MutexLock.h"
#include "Condition.h"

#include <deque>
#include <assert.h>

namespace blink
{

template<typename T>
class BlockingQueue : Nocopyable
{
public:
    BlockingQueue()
        : mutex_(), not_empty_(mutex_), queue_()
    {
    }

    void put(const T& task)
    {
        MutexLockGuard guard(mutex_);
        queue_.push_back(task);
        not_empty_.wakeup();
    }

    T take()
    {
        MutexLockGuard guard(mutex_);
        while(queue_.empty())
        {
            not_empty_.wait();
        }
        assert(!queue_.empty());
        T task = queue_.front();
        queue_.pop_front();
        return task;
    }

    size_t size() const
    {
        MutexLockGuard guard(mutex_);
        return queue_.size();
    }

private:
    mutable MutexLock  mutex_;
    Condition          not_empty_;
    std::deque<T>      queue_;
};

}  // namespace blink

#endif
