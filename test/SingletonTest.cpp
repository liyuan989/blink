#include <blink/Singleton.h>
#include <blink/Thread.h>
#include <blink/CurrentThread.h>

#include <stdio.h>

using namespace blink;

class Singletontest
{
public:
    Singletontest()
        : message_("none")
    {
        printf("constructing,  tid = %d  name: %s  addr: %p\n",
               current_thread::tid(), current_thread::threadName(), this);
    }
    ~Singletontest()
    {
        printf("destroying, message: %s  tid = %d  name: %s addr: %p\n",
               message_.c_str(), current_thread::tid(), current_thread::threadName(), this);
    }

    void setMessage(const string& msg)
    {
        message_ = msg;
    }

    string message() const
    {
        return message_;
    }

private:
    string message_;
};

class SingletonTestNoDestroy
{
public:
    SingletonTestNoDestroy()
    {
        printf("SingletonTestNoDestroy constructing,  tid = %d  name: %s  addr: %p\n",
               current_thread::tid(), current_thread::threadName(), this);
    }
    ~SingletonTestNoDestroy()
    {
        printf("SingletonTestNoDestroy destroying,  tid = %d  name: %s  addr: %p\n",
               current_thread::tid(), current_thread::threadName(), this);
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
    Thread thread(func, string("test Singletontest"));
    thread.start();
    thread.join();

    printf("tid = %d Singletontest::message_ = %s\n", current_thread::tid(),
           Singleton<Singletontest>::getInstance().message().c_str());
    Singleton<SingletonTestNoDestroy>::getInstance();
    printf("sizeof(SingletonTestNoDestroy) = %zd addr = %p\n",
            sizeof(Singleton<SingletonTestNoDestroy>::getInstance()),
            &Singleton<SingletonTestNoDestroy>::getInstance());
    return 0;
}
