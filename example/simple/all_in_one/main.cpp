#include "../chargen/ChargenServer.h"
#include "../daytime/DaytimeServer.h"
#include "../discard/DiscardServer.h"
#include "../echo/EchoServer.h"
#include "../time/TimeServer.h"

#include "EventLoop.h"
#include "InetAddress.h"

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
