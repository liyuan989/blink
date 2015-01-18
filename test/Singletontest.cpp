#include "Singleton.h"
#include "Thread.h"
#include "CurrentThread.h"

#include <string>
#include <stdio.h>

using namespace blink;

class Singletontest
{
public:
    Singletontest()
        : message_("none")
    {
        printf("constructing,  tid = %d  name: %s  addr: %p\n", tid(), threadName(), this);
    }
    ~Singletontest()
    {
        printf("destroying, message: %s  tid = %d  name: %s addr: %p\n", message_.c_str(), tid(), threadName(), this);
    }

    void setMessage(const std::string& message)
    {
        message_ = message;
    }

    std::string message() const
    {
        return message_;
    }

private:
    std::string message_;
};

class SingletonTestNoDestroy
{
public:
    SingletonTestNoDestroy()
    {
        printf("SingletonTestNoDestroy constructing,  tid = %d  name: %s  addr: %p\n", tid(), threadName(), this);
    }
    ~SingletonTestNoDestroy()
    {
        printf("SingletonTestNoDestroy destroying,  tid = %d  name: %s  addr: %p\n", tid(), threadName(), this);
    }

    void no_destroy()
    {
    }
};

void func()
{
    printf("get Singletontest instance, only one: message = %s\n",
           Singleton<Singletontest>::getInstance().message().c_str());
    Singleton<Singletontest>::getInstance().setMessage("changed!");
}

int main(int argc, char const *argv[])
{
    Thread thread(func, std::string("test Singletontest"));
    thread.start();
    thread.join();

    printf("tid = %d Singletontest::message_ = %s\n", tid(),
           Singleton<Singletontest>::getInstance().message().c_str());
    Singleton<SingletonTestNoDestroy>::getInstance();
    printf("sizeof(SingletonTestNoDestroy) = %zd addr = %p\n",
            sizeof(Singleton<SingletonTestNoDestroy>::getInstance()),
            &Singleton<SingletonTestNoDestroy>::getInstance());
    return 0;
}
