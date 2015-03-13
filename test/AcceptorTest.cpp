#include <blink/Acceptor.h>
#include <blink/EventLoop.h>
#include <blink/InetAddress.h>

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

void print(int connectfd, const InetAddress& addr, EventLoop* loop)
{
    printf("connectfd = %d, addr = %s\n", connectfd, addr.toIp().c_str());
    ::close(connectfd);
    loop->quit();
}

int main(int argc, char const *argv[])
{
    EventLoop loop;
    InetAddress local_addr(9600);
    printf("local_addr = %s, port = %hu\n", local_addr.toIpPort().c_str(), local_addr.toPort());
    Acceptor acceptor(&loop, local_addr, false);
    acceptor.setNewConnectionCallback(boost::bind(print, _1, _2, &loop));
    acceptor.listen();
    loop.loop();
    return 0;
}
