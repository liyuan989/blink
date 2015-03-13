#include <blink/EventLoop.h>

int main(int argc, char const *argv[])
{
    blink::EventLoop loop;
    loop.loop();
    return 0;
}
