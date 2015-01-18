#ifndef __BOUNDEDBLOCKINGQUEUE_H__
#define __BOUNDEDBLOCKINGQUEUE_H__

#include "Nocopyable.h"
#include "MutexLock.h"
#include "Condition.h"

#include <boost/circular_buffer.hpp>

#include <assert.h>

namespace blink
{

template<typename T>
class BounededBlockingQueue : Nocopyable
{
public:
    explicit BounededBlockingQueue(size_t maxsize)
        : mutex_(), not_empty_(mutex_), not_full_(mutex_), queue_(maxsize)
    {
    }

    void put(const T& task)
    {
        MutexLockGuard guard(mutex_);
        while (queue_.full())
        {
            not_full_.wait();
        }
        assert(!queue_.full());
        queue_.push_back(task);
        not_empty_.wakeup();
    }

    T take()
    {
        MutexLockGuard guard(mutex_);
        while (queue_.empty())
        {
            not_empty_.wait();
        }
        assert(!queue_.empty());
        T task = queue_.front();
        queue_.pop_front();
        not_full_.wakeup();
        return task;
    }

    bool empty() const
    {
        MutexLockGuard guard(mutex_);
        return queue_.empty();
    }

    bool full() const
    {
        MutexLockGuard guard(mutex_);
        return queue_.full();
    }

    size_t size() const
    {
        MutexLockGuard guard(mutex_);
        return queue_.size();
    }

    size_t capacity() const
    {
        MutexLockGuard guard(mutex_);
        return queue_.capacity();
    }

private:
    mutable MutexLock          mutex_;
    Condition                  not_empty_;
    Condition                  not_full_;
    boost::circular_buffer<T>  queue_;
};

}  // namespace blink

#endif
