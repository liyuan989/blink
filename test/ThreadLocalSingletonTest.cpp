#include "ThreadLocalSingleton.h"
#include "CurrentThread.h"
#include "Thread.h"

#include <boost/bind.hpp>

#include <string>
#include <stdio.h>
#include <time.h>

using namespace blink;

class Test
{
public:
    Test()
    {
        printf("tid = %d, constructing addr: %p\n", tid(), this);
    }

    ~Test()
    {
        printf("tid = %d, destructing addr: %p, Test name: %s\n", tid(), this, name_.c_str());
    }

    void setName(const std::string namemsg)
    {
        name_ = namemsg;
    }

    const std::string& name() const
    {
        return name_;
    }

private:
    std::string name_;
};

void print()
{
    printf("tid = %d, singleton obj name: %s\n",
           tid(), ThreadLocalSingleton<Test>::getInstance().name().c_str());
}

void func(std::string change)
{
    print();
    ThreadLocalSingleton<Test>::getInstance().setName(change);
    timespec sp = {1, 0};
    nanosleep(&sp, NULL);
    print();
}

int main(int argc, char const *argv[])
{
    ThreadLocalSingleton<Test>::getInstance().setName("main set start");
    print();
    Thread t1(boost::bind(func, "t1 changed"));
    Thread t2(boost::bind(func, "t2 changed"));
    t1.start();
    t2.start();
    t1.join();
    t2.join();

    return 0;
}
