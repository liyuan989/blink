#include <blink/ThreadLocal.h>
#include <blink/CurrentThread.h>
#include <blink/Thread.h>

#include <stdio.h>

using namespace blink;

class ThreadLocalTest
{
public:
    ThreadLocalTest()
    {
        printf("Thread tid: %d, Thread name: %s , construct addr: %p\n",
               current_thread::tid(), current_thread::threadName(), this);
    }

    ~ThreadLocalTest()
    {
        printf("Thread tid: %d, Thread name: %s , destruct addr: %p, test name: %s\n",
               current_thread::tid(), current_thread::threadName(), this, name_.c_str());
    }

    void setName(const string namemsg)
    {
        name_ = namemsg;
    }

    const string& name() const
    {
        return name_;
    }

private:
    string  name_;
};

ThreadLocal<ThreadLocalTest> test1;
ThreadLocal<ThreadLocalTest> test2;

void print()
{
    printf("Thread tid: %d, test1 addr: %p, test1 name: %s\n",
           current_thread::tid(), &test1, test1.value().name().c_str());
    printf("Thread tid: %d, test2 addr: %p, test2 name: %s\n",
           current_thread::tid(), &test2, test2.value().name().c_str());
}

void func()
{
    print();
    test1.value().setName("change test1 by subthread.");
    test2.value().setName("change test2 by subthread.");
    print();
}

int main(int argc, char const *argv[])
{
    print();
    test1.value().setName("main change test1");
    test2.value().setName("main change test2");
    Thread t1(func, "sub thread");
    t1.start();
    print();
    t1.join();
    return 0;
}
