#include <blink/Thread.h>
#include <blink/ProcessBase.h>
#include <blink/CurrentThread.h>

#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include <unistd.h>

#include <time.h>
#include <stdio.h>

using namespace blink;

void testThreadFunc1()
{
    printf("Thread1: pid = %d  tid = %d\n", processes::getpid(), tid());
}

void testThreadFunc2(int data)
{
    printf("Thread2: pid = %d  tid = %d  data = %d\n", processes::getpid(), tid(), data);
}

class Test
{
public:
    explicit Test(const string& s)
        : s_(s)
    {
    }

    void testThreadFunc3(const string& msg)
    {
        printf("Thread3: pid = %d  tid = %d  member = %s  msg = %s\n",
               processes::getpid(), tid(), s_.c_str(), msg.c_str());
    }

    void testThreadFunc4()
    {
        sleep(2);
        printf("Thread4: pid = %d  tid = %d  member = %s\n", processes::getpid(), tid(), s_.c_str());
    }

private:
    string s_;
};

int main(int argc, char const *argv[])
{
    printf("main: pid = %d  tid =  %d\n", processes::getpid(), tid());

    Thread thread1(boost::bind(testThreadFunc1), string("test thread1"));
    thread1.start();
    thread1.join();

    Thread thread2(boost::bind(testThreadFunc2, 99), string("test thread2"));
    thread2.start();
    thread2.join();

    Test test3(string("test3"));
    Thread thread3(boost::bind(&Test::testThreadFunc3, &test3, string("message")),
                   string("test thread3"));
    thread3.start();
    thread3.join();

    {
        Test test4(string("test4"));
        Thread thread4(boost::bind(&Test::testThreadFunc4, boost::ref(test4)), string("test thread4"));
        thread4.start();
    }

    struct timespec spec = {1, 0};
    nanosleep(&spec, NULL);

    printf("main has creat %d threads\n", Thread::numCreated());
    return 0;
}
