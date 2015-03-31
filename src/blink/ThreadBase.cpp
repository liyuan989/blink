#include <blink/ThreadBase.h>
#include <blink/Log.h>

#include <sys/syscall.h>
#include <pthread.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>

namespace blink
{

namespace threads
{

pid_t gettid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));  // SYS_gettid equal 224
}

int pthread_create(pthread_t* tid, const pthread_attr_t* attr, void* (*start_routine) (void*), void* arg)
{
    int val = ::pthread_create(tid, attr, start_routine, arg);
    if (val != 0)
    {
        LOG_ERROR << "pthread_creat error: " << strerror(val);
    }
    return val;
}

pthread_t pthread_self()
{
    return ::pthread_self();
}

void pthread_exit(void* thread_return)
{
    ::pthread_exit(thread_return);
}

int pthread_cancel(pthread_t tid)
{
    int val = ::pthread_cancel(tid);
    if (val != 0)
    {
        LOG_ERROR << "pthread_cancesl error: " << strerror(val);
    }
    return val;
}

int pthread_join(pthread_t tid, void** thread_return)
{
    int val = ::pthread_join(tid, thread_return);
    if (val != 0)
    {
        LOG_ERROR << "pthread_join error: " << strerror(val);
    }
    return val;
}

int pthread_detach(pthread_t tid)
{
    int val = ::pthread_detach(tid);
    if (val != 0)
    {
        LOG_ERROR << "pthread_detach error: " << strerror(val);
    }
    return val;
}

int pthread_once(pthread_once_t* once_control, void (*init_routine)())
{
    return ::pthread_once(once_control, init_routine);
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)
{
    int val = ::pthread_mutex_init(mutex, attr);
    if (val != 0)
    {
        LOG_ERROR << "pthread_mutex_init error: " << strerror(val);
    }
    return val;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
    int val = ::pthread_mutex_destroy(mutex);
    if (val != 0)
    {
        LOG_ERROR << "pthread_mutex_destroy error: " << strerror(val);
    }
    return val;
}

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
    int val = ::pthread_mutex_lock(mutex);
    if (val != 0)
    {
        LOG_ERROR << "pthread_mutex_lock error: " << strerror(val);
    }
    return val;
}

int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
    int val = ::pthread_mutex_trylock(mutex);
    if (val != 0 && val != EBUSY)
    {
        LOG_ERROR << "pthread_mutex_trylock error: " << strerror(val);
    }
    return val;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    int val = ::pthread_mutex_unlock(mutex);
    if (val != 0)
    {
        LOG_ERROR << "pthread_mutex_unlock error: " << strerror(val);
    }
    return val;
}

int pthread_cond_init(pthread_cond_t* cond, pthread_condattr_t* attr)
{
    int val = ::pthread_cond_init(cond, attr);
    if (val != 0)
    {
        LOG_ERROR << "pthread_cond_init error: " << strerror(val);
    }
    return val;
}

int pthread_cond_destroy(pthread_cond_t* cond)
{
    int val = ::pthread_cond_destroy(cond);
    if (val != 0)
    {
        LOG_ERROR << "pthread_cond_destroy error: " << strerror(val);
    }
    return val;
}

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex)
{
    int val = ::pthread_cond_wait(cond, mutex);
    if (val != 0)
    {
        LOG_ERROR << "pthread_cond_wait error: " << strerror(val);
    }
    return val;
}

int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec* timeout)
{
    int val = ::pthread_cond_timedwait(cond, mutex, timeout);
    if (val != 0 && val != ETIMEDOUT)
    {
        LOG_ERROR << "pthread_cond_timewait error: " << strerror(val);
    }
    return val;
}

int pthread_cond_signal(pthread_cond_t* cond)
{
    int val = ::pthread_cond_signal(cond);
    if (val != 0)
    {
        LOG_ERROR << "pthread_cond_signal error: " << strerror(val);
    }
    return val;
}

int pthread_cond_broadcast(pthread_cond_t* cond)
{
    int val = ::pthread_cond_broadcast(cond);
    if (val != 0)
    {
        LOG_ERROR << "pthread_cond_broadcast error: " << strerror(val);
    }
    return val;
}

int pthread_atfork(void (*prepare)(), void (*parent)(), void (*child)())
{
    int val = ::pthread_atfork(prepare, parent, child);
    if (val != 0)
    {
        LOG_ERROR << "pthread_atfork error: " << strerror(val);
    }
    return val;
}

int pthread_key_create(pthread_key_t* key, void (*destructor)(void*))
{
    int val = ::pthread_key_create(key, destructor);
    if (val != 0)
    {
        LOG_ERROR << "pthread_key_create error: " << strerror(val);
    }
    return val;
}

int pthread_key_delete(pthread_key_t key)
{
    int val = ::pthread_key_delete(key);
    if (val != 0)
    {
        LOG_ERROR << "pthread_key_delete error: " << strerror(val);
    }
    return val;
}

void* pthread_getspecific(pthread_key_t key)
{
    return ::pthread_getspecific(key);
}

int pthread_setspecific(pthread_key_t key, const void* value)
{
    int val = ::pthread_setspecific(key, value);
    if (val != 0)
    {
        LOG_ERROR << "pthread_setspecific error: " << strerror(val);
    }
    return val;
}

}  // namespace threads

}  // namespace blink
