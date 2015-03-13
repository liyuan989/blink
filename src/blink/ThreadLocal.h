#ifndef __BLINK_THREADLOCAL_H__
#define __BLINK_THREADLOCAL_H__

#include <blink/Nocopyable.h>
#include <blink/ThreadBase.h>

namespace blink
{

template<typename T>
class ThreadLocal : Nocopyable
{
public:
    ThreadLocal()
    {
        threads::pthread_key_create(&key_, &ThreadLocal<T>::destructor);
    }

    ~ThreadLocal()
    {
        threads::pthread_key_delete(key_);
    }

    T& value()
    {
        T* val = static_cast<T*>(threads::pthread_getspecific(key_));
        if (!val)
        {
            val = new T();
            threads::pthread_setspecific(key_, val);
        }
        return *val;
    }

private:
    static void destructor(void* arg)
    {
        T* data = static_cast<T*>(arg);
        typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
        T_must_be_complete_type dummy;
        (void)dummy;
        delete data;
    }

    pthread_key_t  key_;
};

}  // namespace blink

#endif
