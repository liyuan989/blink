#include "EventLoop.h"

#include <boost/bind.hpp>

#include <iostream>

void print(blink::EventLoop& loop, int& count)
{
    if (count < 5)
    {
        std::cout << count << std::endl;
        ++count;
        loop.runAfter(1.0, boost::bind(print, boost::ref(loop), boost::ref(count)));
    }
    else
    {
        loop.quit();
    }
}

int main(int argc, char const *argv[])
{
    blink::EventLoop loop;
    int count = 0;
    loop.runAfter(1.0, boost::bind(print, boost::ref(loop), boost::ref(count)));
    loop.loop();
    std::cout << "Final count is " << count << std::endl;
    return 0;
}
