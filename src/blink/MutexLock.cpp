#include <blink/MutexLock.h>
#include <blink/ThreadBase.h>
#include <blink/CurrentThread.h>

namespace blink
{

MutexLock::MutexLock()
    : holder_(0)
{
    MCHECK(threads::pthread_mutex_init(&mutex_, NULL));
}

MutexLock::~MutexLock()
{
    MCHECK(threads::pthread_mutex_destroy(&mutex_));
}

void MutexLock::lock()
{
    MCHECK(threads::pthread_mutex_lock(&mutex_));
    setHolder();
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
    MCHECK(threads::pthread_mutex_unlock(&mutex_));
}

bool MutexLock::isLockedByCurrentThread() const
{
    return holder_ == current_thread::tid();
}

void MutexLock::assertLockted() const
{
    assert(isLockedByCurrentThread());
}

void MutexLock::setHolder()
{
    holder_ = current_thread::tid();
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
