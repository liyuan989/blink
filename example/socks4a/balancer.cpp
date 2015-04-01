#include <example/socks4a/tunnel.h>

#include <blink/ThreadLocal.h>

#include <stdio.h>

using namespace blink;

std::vector<InetAddress> g_backends;
ThreadLocal<std::map<string, TunnelPtr> > t_tunnels;
MutexLock g_mutex;
size_t g_current = 0;

void onServerConnection(const TcpConnectionPtr& connection)
{
    LOG_DEBUG << (connection->connected() ? "UP" : "DOWN");
    std::map<string, TunnelPtr>& tunnels = t_tunnels.value();
    if (connection->connected())
    {
        connection->setTcpNoDelay(true);
        size_t current = 0;
        {
            MutexLockGuard guard(g_mutex);
            current = g_current;
            g_current = (g_current + 1) % g_backends.size();
        }
        InetAddress backend = g_backends[current];
        TunnelPtr tunnel(new Tunnel(connection->getLoop(), backend, connection));
        tunnel->setup();
        tunnel->connect();
        tunnels[connection->name()] = tunnel;
    }
    else
    {
        assert(tunnels.find(connection->name()) != tunnels.end());
        tunnels[connection->name()]->disconnect();
        tunnels.erase(connection->name());
    }
}

void onServerMessage(const TcpConnectionPtr& connection,
                     Buffer* buf,
                     Timestamp receive_time)
{
    if (!connection->getContext().empty())
    {
        const TcpConnectionPtr& client_connection =
            boost::any_cast<TcpConnectionPtr>(connection->getContext());
        client_connection->send(buf);
    }
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <listen_port> <backend_ip:port> [backend_ip:port] ...\n", argv[0]);
        return 1;
    }
    for (int i = 2; i < argc; ++i)
    {
        string host_port = argv[i];
        size_t colon = host_port.find(':');
        if (colon != string::npos)
        {
            string ip = host_port.substr(0, colon);
            uint16_t port = static_cast<uint16_t>(atoi(host_port.c_str() + colon + 1));
            g_backends.push_back(InetAddress(ip, port));
        }
        else
        {
            fprintf(stderr, "Invalid backend address: %s\n", argv[i]);
            return 1;
        }
    }
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress listen_addr(port);
    EventLoop loop;
    TcpServer server(&loop, listen_addr, "TcpBalancer");
    server.setConnectionCallback(onServerConnection);
    server.setMessageCallback(onServerMessage);
    server.setThreadNumber(4);
    server.start();
    loop.loop();
    return 0;
}
