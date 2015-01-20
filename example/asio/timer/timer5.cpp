#include "Nocopyable.h"
#include "EventLoopThread.h"
#include "EventLoop.h"
#include "MutexLock.h"

#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

#include <iostream>

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
        std::cout << "Final count is " << count_ << std::endl;
    }

private:
    void print1()
    {
        blink::MutexLockGuard guard(mutex_);
        if (count_ < 10)
        {
            std::cout << "Timer 1: " << count_ << std::endl;
            ++count_;
            loop1_->runAfter(1.0, boost::bind(&Printer::print1, this));
        }
        else
        {
            loop1_->quit();
        }
    }

    void print2()
    {
        blink::MutexLockGuard guard(mutex_);
        if (count_ < 10)
        {
            std::cout << "Timer 2: " << count_ << std::endl;
            ++count_;
            loop2_->runAfter(1.0, boost::bind(&Printer::print2, this));
        }
        else
        {
            loop2_->quit();
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
