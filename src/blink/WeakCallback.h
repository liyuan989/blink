#ifndef __BLINK_WEAKCALLBACK_H__
#define __BLINK_WEAKCALLBACK_H__

#include <blink/Copyable.h>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace blink
{

template<typename T>
class WeakCallback : Copyable
{
public:
    WeakCallback(const boost::weak_ptr<T>& arg, const boost::function<void (T*)>& function)
        : arg_(arg), function_(function)
    {
    }

    void operator()()
    {
        boost::shared_ptr<T> ptr(arg_.lock());
        if (ptr)
        {
            function_(ptr.get());
        }
    }

private:
    boost::weak_ptr<T>          arg_;
    boost::function<void (T*)>  function_;
};

template<typename T>
WeakCallback<T> makeWeakCallback(const boost::shared_ptr<T>& arg, void (T::*function)())
{
    return WeakCallback<T>(arg, function);
}

template<typename T>
WeakCallback<T> makeWeakCallback(const boost::shared_ptr<T>& arg, void (T::*function)() const)
{
    return WeakCallback<T>(arg, function);
}

template<typename T>
WeakCallback<T> makeWeakCallback(const boost::shared_ptr<T>& arg, void (*function)(T*))
{
    return WeakCallback<T>(arg, function);
}

}  // namespace blink

#endif
