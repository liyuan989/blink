#include <blink/Channel.h>
#include <blink/Socket.h>
#include <blink/EventLoop.h>
#include <blink/SocketBase.h>
#include <blink/InetAddress.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

const size_t kFramLen = 2 * sizeof(int64_t);

int createNonblockingUdp()
{
    int sockfd = sockets::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
    if (sockfd < 0)
    {
        LOG_FATAL << "createNonblockingUdp()";
    }
    return sockfd;
}

void serverReadCallback(int sockfd, Timestamp receive_time)
{
    int64_t message[2];
    struct sockaddr peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    socklen_t addrlen = sizeof(peer_addr);
    ssize_t nread = ::recvfrom(sockfd, message, sizeof(message), 0, &peer_addr, &addrlen);
    char addr_str[32];
    sockets::toIpPort(addr_str, sizeof(addr_str), *sockets::sockaddr_in_cast(&peer_addr));
    LOG_DEBUG << "received " << nread << " bytes from " << addr_str;
    if (nread < 0)
    {
        LOG_ERROR << "::recvfrom";
    }
    else if (static_cast<size_t>(nread) == kFramLen)
    {
        message[1] = receive_time.microSecondsSinceEpoch();
        ssize_t nwrite = ::sendto(sockfd, message, sizeof(message), 0, &peer_addr, addrlen);
        if (nwrite < 0)
        {
            LOG_ERROR << "::sendto";
        }
        else if (static_cast<size_t>(nwrite) != kFramLen)
        {
            LOG_ERROR << "expect " << kFramLen << " bytes, wrote " << nwrite << " bytes.";
        }
    }
    else
    {
        LOG_ERROR << "expect " << kFramLen << " bytes, received " << nread << " bytes.";
    }
}

void runServer(uint16_t port)
{
    Socket socket(createNonblockingUdp());
    socket.bindAddress(InetAddress(port));
    EventLoop loop;
    Channel channel(&loop, socket.fd());
    channel.setReadCallback(boost::bind(serverReadCallback, socket.fd(), _1));
    channel.enableReading();
    loop.loop();
}

void clientReadCallback(int sockfd, Timestamp receive_time)
{
    int64_t message[2];
    ssize_t nread = sockets::read(sockfd, message, sizeof(message));
    if (nread < 0)
    {
        LOG_ERROR << "sockets::read";
    }
    else if (static_cast<size_t>(nread) == kFramLen)
    {
        int64_t send = message[0];
        int64_t their = message[1];
        int64_t back = receive_time.microSecondsSinceEpoch();
        int64_t mine = (back + send) / 2;
        LOG_INFO << "round trip " << back - send
                 << " clock error " << their - mine;
    }
    else
    {
        LOG_ERROR << "expect " << kFramLen << " bytes, received " << nread << " bytes.";
    }
}

void sendMyTime(int sockfd)
{
    int64_t message[2] = {0, 0};
    message[0] = Timestamp::now().microSecondsSinceEpoch();
    ssize_t nwrite = sockets::write(sockfd, message, sizeof(message));
    if (nwrite < 0)
    {
        LOG_ERROR << "sockets::write";
    }
    else if (static_cast<size_t>(nwrite) != kFramLen)
    {
        LOG_ERROR << "expect " << kFramLen << " bytes, wrote " << nwrite << " bytes.";
    }
}

void runClient(const char* ip, uint16_t port)
{
    Socket socket(createNonblockingUdp());
    InetAddress server_addr(ip, port);
    struct sockaddr_in addr = server_addr.getSockAddrInet();
    int ret = sockets::connect(socket.fd(), sockets::sockaddr_cast(&addr), sizeof(addr));
    if (ret < 0)
    {
        LOG_FATAL << "sockets::connect";
    }
    EventLoop loop;
    Channel channel(&loop, socket.fd());
    channel.setReadCallback(boost::bind(clientReadCallback, socket.fd(), _1));
    channel.enableReading();
    loop.runEvery(0.2, boost::bind(sendMyTime, socket.fd()));
    loop.loop();
}

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        fprintf(stderr, "usage: %s -s <port>\n", argv[0]);
        return 1;
    }
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    if (strcmp(argv[1], "-s") == 0)
    {
        runServer(port);
    }
    else
    {
        runClient(argv[1], port);
    }
    return 0;
}
