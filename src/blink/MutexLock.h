#ifndef __BLINK_MUTEXLOCK_H__
#define __BLINK_MUTEXLOCK_H__

#include <blink/Nocopyable.h>

#include <pthread.h>

#include <assert.h>

#ifdef CHECK_PTHREAD_RETURN_VALUE

#ifdef NDEBUG

__BEGIN_DECLS
extern void __assert_perror_fail(int errnum,
                                const char* file,
                                unsigned int line,
                                const char* function)
        __THROW __attribute__ ((__noreturn__));
__END_DECLS

#endif // NDEBUG

#define MCHECK(ret)                                                  \
{                                                                    \
    __typeof__(ret) errnum = (ret);                                  \
    if (__builtin_expect(errnum != 0, 0))                            \
    {                                                                \
        __assert_perror_fail(errnum, __FILE__, __LINE__, __func__);  \
    }                                                                \
}

#else  // CHECK_PTHREAD_RETURN_VALUE

#define MCHECK(ret)                  \
{                                    \
    __typeof__(ret) errnum = (ret);  \
    assert(errnum == 0);             \
    (void)errnum;                    \
}

#endif  // CHECK_PTHREAD_RETURN_VALUE

namespace blink
{

class MutexLock : Nocopyable
{
public:
    MutexLock();
    ~MutexLock();

    void lock();
    void trylock();
    void unlock();
    bool isLockedByCurrentThread() const;
    void assertLockted() const;

    bool isLocked() const
    {
        return !(holder_ == 0);
    }

    pthread_mutex_t* getMutex() // non-const
    {
        return &mutex_;
    }

private:
    friend class Condition;

    class ConditionGuard
    {
    public:
        ConditionGuard(MutexLock& mutex)
            : guarder_(mutex)
        {
            guarder_.resetHolder();
        }

        ~ConditionGuard()
        {
            guarder_.setHolder();
        }

    private:
        MutexLock&  guarder_;
    };

    void setHolder();

    void resetHolder()
    {
        holder_ = 0;
    }

    pthread_mutex_t  mutex_;
    pid_t            holder_;
};

class MutexLockGuard : Nocopyable
{
public:
    explicit MutexLockGuard(MutexLock& mutex);
    ~MutexLockGuard();

private:
    MutexLock& mutex_;
};

}  // namespace blink

#endif
