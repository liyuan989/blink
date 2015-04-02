#include <example/socks4a/tunnel.h>

#include <blink/Endian.h>

#include <netdb.h>

#include <stdio.h>

using namespace blink;

EventLoop* g_loop = NULL;
std::map<string, TunnelPtr> g_tunnels;

void onServerConnection(const TcpConnectionPtr& connection)
{
    LOG_DEBUG << connection->name() << (connection->connected() ? " UP" : " DOWN");
    if (connection->connected())
    {
        connection->setTcpNoDelay(true);
    }
    else
    {
        std::map<string, TunnelPtr>::iterator it = g_tunnels.find(connection->name());
        if (it != g_tunnels.end())
        {
            it->second->disconnect();
            g_tunnels.erase(it);
        }
    }
}

void onServerMessage(const TcpConnectionPtr& connection,
                     Buffer* buf,
                     Timestamp receive_time)
{
    LOG_DEBUG << connection->name() << " " << buf->readableSize();
    if (g_tunnels.find(connection->name()) == g_tunnels.end())
    {
        if (buf->readableSize() > 128)
        {
            connection->shutdown();
        }
        else if (buf->readableSize() > 8)
        {
            const char* begin = buf->peek() + 8;
            const char* end = buf->peek() + buf->readableSize();
            const char* where = std::find(begin, end, '\0');
            if (where != end)
            {
                char ver = buf->peek()[0];
                char cmd = buf->peek()[1];
                const void* port = buf->peek() + 2;
                const void* ip = buf->peek() + 4;

                sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = *static_cast<const in_port_t*>(port);
                addr.sin_addr.s_addr = *static_cast<const uint32_t*>(ip);

                bool socks4a = sockets::networkToHost32(addr.sin_addr.s_addr) < 256;
                bool okay = false;
                if (socks4a)
                {
                    const char* end_of_hostname = std::find(where + 1, end, '\0');
                    if (end_of_hostname != end)
                    {
                        string hostname = where + 1;
                        where = end_of_hostname;
                        LOG_INFO << "socks4a host name: " << hostname;
                        InetAddress temp;
                        if (InetAddress::resolve(hostname, &temp))
                        {
                            addr.sin_addr.s_addr = temp.ipNetEndian();
                            okay = true;
                        }
                    }
                    else
                    {
                        return;
                    }
                }
                else
                {
                    okay = true;
                }

                InetAddress server_addr(addr);
                if (ver == 4 && cmd == 1 && okay)
                {
                    TunnelPtr tunnel(new Tunnel(g_loop, server_addr, connection));
                    tunnel->setup();
                    tunnel->connect();
                    g_tunnels[connection->name()] = tunnel;
                    buf->resetUntil(where + 1);
                    char response[] = "\000\x5aUVWXYZ";
                    memcpy(response + 2, &addr.sin_port, 2);
                    memcpy(response + 4, &addr.sin_addr.s_addr, 4);
                    connection->send(response, 8);
                }
                else
                {
                    char response[] = "\000\x5bUVWXYZ";
                    connection->send(response, 8);
                    connection->shutdown();
                }
            }
        }
    }
    else if (!connection->getContext().empty())
    {
        const TcpConnectionPtr& client_connection =
            boost::any_cast<TcpConnectionPtr>(connection->getContext());
        client_connection->send(buf);
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <listen_port>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid() << ", tid = " << current_thread::tid();
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress listen_addr(port);
    EventLoop loop;
    g_loop = &loop;
    TcpServer server(&loop, listen_addr, "Socks4a");
    server.setConnectionCallback(onServerConnection);
    server.setMessageCallback(onServerMessage);
    server.start();
    loop.loop();
    return 0;
}
