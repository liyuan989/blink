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
    threads::pthread_cond_init(&cond_, NULL);
}

Condition::~Condition()
{
    threads::pthread_cond_destroy(&cond_);
}

void Condition::wait()
{
    MutexLock::ConditionGuard guard(mutex_);
    threads::pthread_cond_wait(&cond_, mutex_.getMutex());
}

int Condition::timedWait(int seconds)
{
    struct timespec absolute_time;
    clock_gettime(CLOCK_REALTIME, &absolute_time);
    absolute_time.tv_sec += seconds;
    MutexLock::ConditionGuard guard(mutex_);
    return threads::pthread_cond_timedwait(&cond_, mutex_.getMutex(), &absolute_time);
}

void Condition::wakeup()
{
    threads::pthread_cond_signal(&cond_);
}

void Condition::wakeupAll()
{
    threads::pthread_cond_broadcast(&cond_);
}

}  // namespace blink
