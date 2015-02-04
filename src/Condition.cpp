#include "ThreadBase.h"
#include "Condition.h"
#include "MutexLock.h"

#include <time.h>
#include <errno.h>

namespace blink
{

Condition::Condition(MutexLock& mutex)
    : mutex_(mutex)
{
    MCHECK(threads::pthread_cond_init(&cond_, NULL));
}

Condition::~Condition()
{
    MCHECK(threads::pthread_cond_destroy(&cond_));
}

void Condition::wait()
{
    MutexLock::ConditionGuard guard(mutex_);
    MCHECK(threads::pthread_cond_wait(&cond_, mutex_.getMutex()));
}

bool Condition::timedWait(int seconds)
{
    struct timespec absolute_time;
    clock_gettime(CLOCK_REALTIME, &absolute_time);
    absolute_time.tv_sec += seconds;
    MutexLock::ConditionGuard guard(mutex_);
    return ETIMEDOUT == threads::pthread_cond_timedwait(&cond_, mutex_.getMutex(), &absolute_time);
}

void Condition::wakeup()
{
    MCHECK(threads::pthread_cond_signal(&cond_));
}

void Condition::wakeupAll()
{
    MCHECK(threads::pthread_cond_broadcast(&cond_));
}

}  // namespace blink
