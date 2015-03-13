#include "example/simple/chargen/ChargenServer.h"
#include "example/simple/daytime/DaytimeServer.h"
#include "example/simple/discard/DiscardServer.h"
#include "example/simple/echo/EchoServer.h"
#include "example/simple/time/TimeServer.h"

#include <blink/EventLoop.h>
#include <blink/InetAddress.h>

int main(int argc, char const *argv[])
{
    blink::EventLoop loop;

    ChargenServer chargen_server(&loop, blink::InetAddress(9600), "ChargenServer");
    chargen_server.start();

    DaytimeServer daytime_server(&loop, blink::InetAddress(9601), "DaytimeServer");
    daytime_server.start();

    DiscardServer discard_server(&loop, blink::InetAddress(9602), "DiscardServer");
    discard_server.start();

    EchoServer echo_server(&loop, blink::InetAddress(9603), "EchoServer");
    echo_server.start();

    TimeServer time_server(&loop, blink::InetAddress(9604), "TimeServer");
    time_server.start();

    loop.loop();
    return 0;
}
