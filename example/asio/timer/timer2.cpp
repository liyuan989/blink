#include "EventLoop.h"

#include <boost/bind.hpp>

#include <iostream>

void print(blink::EventLoop& loop)
{
    std::cout << "hey boy!" << std::endl;
    loop.quit();
}

int main(int argc, char* argv[])
{
    blink::EventLoop loop;
    loop.runAfter(3.0, boost::bind(print, boost::ref(loop)));
    loop.loop();
    return 0;
}
