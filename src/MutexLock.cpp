#include "MutexLock.h"
#include "ThreadBase.h"
#include "CurrentThread.h"
#include <stdio.h>
namespace blink
{

MutexLock::MutexLock()
    : holder_(0)
{
    threads::pthread_mutex_init(&mutex_, NULL);
}

MutexLock::~MutexLock()
{
    threads::pthread_mutex_destroy(&mutex_);
}

void MutexLock::lock()
{
    if (threads::pthread_mutex_lock(&mutex_) == 0)
    {
        setHolder();
    }
}

void MutexLock::trylock()
{
    if (threads::pthread_mutex_trylock(&mutex_) == 0)
    {
        setHolder();
    }
}

void MutexLock::unlock()
{
    resetHolder();
    threads::pthread_mutex_unlock(&mutex_);
}

bool MutexLock::isLockedByCurrentThread() const
{
    return holder_ == blink::tid();
}

void MutexLock::setHolder()
{
    holder_ = blink::tid();
}

MutexLockGuard::MutexLockGuard(MutexLock& mutex)
    : mutex_(mutex)
{
    mutex_.lock();
}

MutexLockGuard::~MutexLockGuard()
{
    mutex_.unlock();
}

}  // namespace blink
