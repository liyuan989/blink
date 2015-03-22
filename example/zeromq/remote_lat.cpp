#include <example/zeromq/codec.h>

#include <blink/TcpClient.h>
#include <blink/EventLoop.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

bool g_tcpNoDelay = false;
int g_messageSize = 0;
int g_totalMessages = 0;
int g_messageCount = 0;
string g_message;
Timestamp g_start;

void onConnection(Codec* codec, const TcpConnectionPtr& connecton)
{
    if (connecton->connected())
    {
        LOG_INFO << "connected";
        g_start = Timestamp::now();
        connecton->setTcpNoDelay(g_tcpNoDelay);
        codec->send(boost::get_pointer(connecton), g_message);
    }
    else
    {
        LOG_INFO << "disconnected";
        EventLoop::getEventLoopOfCurrentThread()->quit();
    }
}

void onStringMessage(Codec* codec,
                     const TcpConnectionPtr& connection,
                     const string& message,
                     Timestamp receive_time)
{
    if (message.size() != static_cast<size_t>(g_messageSize))
    {
        abort();
    }
    ++g_messageCount;
    if (g_messageCount < g_totalMessages)
    {
        codec->send(boost::get_pointer(connection), message);
    }
    else
    {
        Timestamp end = Timestamp::now();
        LOG_INFO << "done";
        double elapsed = timeDifference(end, g_start);
        LOG_INFO << g_messageSize << " message bytes";
        LOG_INFO << g_messageCount << " round-trips";
        LOG_INFO << elapsed << " seconds";
        LOG_INFO << Format("%.3f", g_messageCount / elapsed) << " round-trips per seconds";
        LOG_INFO << Format("%.3f", (10000000 * elapsed / g_messageCount / 2)) << " latency[us]";
        LOG_INFO << Format("%.3f", (g_messageSize * g_messageCount / elapsed / 1024 / 1024))
                 << " band with [MiB/s]";
        connection->shutdown();
    }
}

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <ipaddr> <port> <msg_size> [msg_count] [tcp_no_delay]\n", argv[0]);
        return 1;
    }
    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    g_messageSize = atoi(argv[3]);
    g_message.assign(g_messageSize, 'L');
    g_totalMessages = argc > 4 ? atoi(argv[4]) : 10000;
    g_tcpNoDelay = argc > 5 ? atoi(argv[5]) : 0;

    EventLoop loop;
    InetAddress server_addr(ip, port);
    TcpClient client(&loop, server_addr, "PingPong-Client");
    Codec codec(boost::bind(onStringMessage, &codec, _1, _2, _3));
    client.setConnectionCallback(boost::bind(onConnection, &codec, _1));
    client.setMessageCallback(boost::bind(&Codec::onMessage, &codec, _1, _2, _3));
    client.connect();
    loop.loop();
    return 0;
}
