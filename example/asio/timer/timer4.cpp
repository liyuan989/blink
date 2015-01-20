#include "Nocopyable.h"
#include "EventLoop.h"

#include <boost/bind.hpp>

#include <iostream>

class Printer : blink::Nocopyable
{
public:
    Printer(blink::EventLoop* loop)
        : loop_(loop), count_(0)
    {
        loop_->runAfter(1.0, boost::bind(&Printer::print, this));
    }

    ~Printer()
    {
        std::cout << "Final count is " << count_ << std::endl;
    }

private:
    void print()
    {
        if (count_ < 5)
        {
            std::cout << count_ << std::endl;
            ++count_;
            loop_->runAfter(1.0, boost::bind(&Printer::print, this));
        }
        else
        {
            loop_->quit();
        }
    }

    blink::EventLoop*  loop_;
    int                count_;
};

int main(int argc, char const *argv[])
{
    blink::EventLoop loop;
    Printer print(&loop);
    loop.loop();
    return 0;
}
