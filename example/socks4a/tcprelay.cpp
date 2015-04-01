#include <example/socks4a/tunnel.h>

#include <sys/resource.h>
#include <malloc.h>

#include <stdio.h>

using namespace blink;

EventLoop* g_loop = NULL;
InetAddress* g_serverAddr = NULL;
std::map<string, TunnelPtr> g_tunnels;

void onServerConnection(const TcpConnectionPtr& connection)
{
    LOG_DEBUG << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        connection->setTcpNoDelay(true);
        TunnelPtr tunnel(new Tunnel(g_loop, *g_serverAddr, connection));
        tunnel->setup();
        tunnel->connect();
        g_tunnels[connection->name()] = tunnel;
    }
    else
    {
        assert(g_tunnels.find(connection->name()) != g_tunnels.end());
        g_tunnels[connection->name()]->disconnect();
        g_tunnels.erase(connection->name());
    }
}

void onServerMessage(const TcpConnectionPtr& connection,
                     Buffer* buf,
                     Timestamp receive_time)
{
    LOG_DEBUG << buf->readableSize();
    if (!connection->getContext().empty())
    {
        const TcpConnectionPtr& client_connection =
            boost::any_cast<TcpConnectionPtr>(connection->getContext());
        client_connection->send(buf);
    }
}

void memstat()
{
    malloc_stats();
}

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <ip> <port> <listen_port>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    {
        // set max virtual memory to 256MB
        size_t kOneMB = 1024 * 1024;
        rlimit rl = {256 * kOneMB, 256 * kOneMB};
        setrlimit(RLIMIT_AS, &rl);
    }
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    uint16_t listen_port = static_cast<uint16_t>(atoi(argv[3]));
    InetAddress server_addr(argv[1], port);
    g_serverAddr = &server_addr;

    EventLoop loop;
    g_loop = &loop;
    loop.runEvery(3.0, memstat);

    TcpServer server(&loop, InetAddress(listen_port), "TcpRelay");
    server.setConnectionCallback(onServerConnection);
    server.setMessageCallback(onServerMessage);
    server.start();
    loop.loop();
    return 0;
}
