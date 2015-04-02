#ifndef __BLINK_SINGLETON_H__
#define __BLINK_SINGLETON_H__

#include <blink/Nocopyable.h>
#include <blink/ThreadBase.h>

#include <pthread.h>

#include <assert.h>
#include <stdlib.h>

namespace blink
{

namespace detail
{

template<typename T>
struct has_no_destroy     // can not check inherited member function.
{                         // http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions
    template<typename _Tp>
    static char check_no_destroy(typeof(&_Tp::no_destroy));

    template<typename _Tp>
    static int check_no_destroy(...);

    static const bool value = (sizeof(check_no_destroy<T>(0)) == 1);
};

template<typename T>
const bool has_no_destroy<T>::value;

}  // namespace detail

template<typename T>
class Singleton : Nocopyable
{
public:
    static T& getInstance()
    {
        threads::pthread_once(&once_control_, init);
        assert(value_ != NULL);
        return *value_;
    }

private:
    static void init()
    {
        if (!detail::has_no_destroy<T>::value)
        {
            value_ = new T();
            ::atexit(destroy);
        }
    }

    static void destroy()
    {
        typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
        T_must_be_complete_type dummy;
        (void)dummy;
        delete value_;
        value_ = NULL;
    }

    Singleton();
    ~Singleton();

    static pthread_once_t  once_control_;
    static T*              value_;
};

template<typename T>
pthread_once_t Singleton<T>::once_control_ = PTHREAD_ONCE_INIT;

template<typename T>
T* Singleton<T>::value_ = NULL;

}  // namespace blink

#endif
