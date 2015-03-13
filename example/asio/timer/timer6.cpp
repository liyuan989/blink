#include <blink/Nocopyable.h>
#include <blink/EventLoopThread.h>
#include <blink/EventLoop.h>
#include <blink/MutexLock.h>

#include <boost/bind.hpp>

//#include <iostream>
#include <stdio.h>

class Printer : blink::Nocopyable
{
public:
    Printer(blink::EventLoop* loop1, blink::EventLoop* loop2)
        : loop1_(loop1), loop2_(loop2), count_(0)
    {
        loop1_->runAfter(1.0, boost::bind(&Printer::print1, this));
        loop2_->runAfter(1.0, boost::bind(&Printer::print2, this));
    }

    ~Printer()
    {
        // std::cout is not thread safe.
        // std::cout << "Final count is " << count_ << std::endl;
        printf("Final count is %d\n", count_);
    }

private:
    void print1()
    {
        bool should_quit = false;
        int count = 0;
        {
            blink::MutexLockGuard guard(mutex_);
            if (count_ < 10)
            {
                count = count_;
                ++count_;
            }
            else
            {
                should_quit = true;
            }
        }
        // out of lock.
        if (should_quit)
        {
            loop1_->quit();
        }
        else
        {
            // std::cout is not thread safe.
            // std::cout << "Timer 1: " << count << std::endl;
            printf("Timer 1: %d\n", count);
            loop1_->runAfter(1.0, boost::bind(&Printer::print1, this));
        }
    }

    void print2()
    {
        bool should_quit = false;
        int count = 0;
        {
            blink::MutexLockGuard guard(mutex_);
            if (count_ < 10)
            {
                count = count_;
                ++count_;
            }
            else
            {
                should_quit = true;
            }
        }
        // out of lock.
        if (should_quit)
        {
            loop2_->quit();
        }
        else
        {
            // std::cout is not thread safe.
            // std::cout << "Timer 1: " << count << std::endl;
            printf("Timer 2: %d\n", count);
            loop2_->runAfter(1.0, boost::bind(&Printer::print2, this));
        }
    }

    blink::EventLoop*  loop1_;
    blink::EventLoop*  loop2_;
    blink::MutexLock   mutex_;
    int                count_;
};

int main(int argc, char const *argv[])
{
    // The scoped_ptr make sure Printer lives than two loops,
    // to avoid race condition of calling print2() on destructed Printer object.
    boost::scoped_ptr<Printer> printer;
    blink::EventLoop loop;
    blink::EventLoopThread loop_thread;
    blink::EventLoop* loop_in_another_thread = loop_thread.startLoop();
    printer.reset(new Printer(&loop, loop_in_another_thread));
    loop.loop();
    return 0;
}
