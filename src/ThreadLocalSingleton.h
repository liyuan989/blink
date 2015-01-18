#ifndef __BLINK_THREADLOCALSINGLETON_H__
#define __BLINK_THREADLOCALSINGLETON_H__

#include "Nocopyable.h"
#include "ThreadBase.h"

#include <assert.h>

namespace blink
{

template<typename T>
class ThreadLocalSingleton : Nocopyable
{
public:
    static T& getInstance()
    {
        if (!t_value_)
        {
            t_value_ = new T();
            deleter_.set(t_value_);
        }
        return *t_value_;
    }

    static T* pointer()
    {
        return t_value_;
    }

private:
    ThreadLocalSingleton();
    ~ThreadLocalSingleton();

    static void destructor(void* arg)
    {
        assert(arg == t_value_);
        typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
        T_must_be_complete_type dummy;
        (void)dummy;
        delete t_value_;
        t_value_ = NULL;
    }

    class Deleter
    {
    public:
        Deleter()
        {
            threads::pthread_key_create(&key_, &ThreadLocalSingleton<T>::destructor);
        }

        ~Deleter()
        {
            threads::pthread_key_delete(key_);
        }

        void set(T* val)
        {
            assert(threads::pthread_getspecific(key_) == NULL);
            threads::pthread_setspecific(key_, val);
        }

        pthread_key_t  key_;
    };

    static __thread T*  t_value_;
    static Deleter      deleter_;
};

template<typename T>
__thread T* ThreadLocalSingleton<T>::t_value_ = NULL;

template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

}  // namespace blink

#endif
