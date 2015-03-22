#include <example/zeromq/codec.h>

#include <blink/EventLoop.h>
#include <blink/TcpServer.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

bool g_tcpNoDelay = false;

void onConnection(const TcpConnectionPtr& connection)
{
    if (connection->connected())
    {
        connection->setTcpNoDelay(g_tcpNoDelay);
    }
}

void onStringMessage(Codec* codec,
                     const TcpConnectionPtr& connection,
                     const string& message,
                     Timestamp receive_time)
{
    codec->send(boost::get_pointer(connection), message);
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <listen_port> [tcp_no_delay] [threads_number]\n", argv[0]);
        return 1;
    }
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    g_tcpNoDelay = argc > 2 ? atoi(argv[2]) : 0;
    int thread_count = argc > 3 ? atoi(argv[3]) : 0;
    LOG_INFO << "pid = " << getpid() << ", listem port = " <<port;

    EventLoop loop;
    InetAddress listen_addr(port);
    TcpServer server(&loop, listen_addr, "PingPong-Server");
    Codec codec(boost::bind(onStringMessage, &codec, _1, _2, _3));
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(boost::bind(&Codec::onMessage, &codec, _1, _2, _3));
    if (thread_count > 1)
    {
        server.setThreadNumber(thread_count);
    }
    server.start();
    loop.loop();
    return 0;
}
