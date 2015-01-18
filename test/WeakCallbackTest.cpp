#include "WeakCallback.h"

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <stdio.h>

class WeakCallbackTest
{
public:
    void foo()
    {
        printf("WeakCallbackTest non-const member function\n");
    }

    void bar() const
    {
        printf("WeakCallbackTest const member function\n");
    }
};

void get(WeakCallbackTest* ptr)
{
    printf("WeakCallbackTest non-member function, ptr addr: %p\n", ptr);
}

int main(int argc, char const *argv[])
{
    boost::shared_ptr<WeakCallbackTest> mem_func(new WeakCallbackTest);
    blink::makeWeakCallback(mem_func, &WeakCallbackTest::foo)();

    boost::shared_ptr<WeakCallbackTest> mem_func_const(new WeakCallbackTest);
    blink::makeWeakCallback(mem_func_const, &WeakCallbackTest::bar)();

    boost::shared_ptr<WeakCallbackTest> non_mem_func(new WeakCallbackTest);
    blink::makeWeakCallback(non_mem_func, get)();
    return 0;
}
