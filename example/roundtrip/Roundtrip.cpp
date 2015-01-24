#include "TcpServer.h"
#include "TcpClient.h"
#include "EventLoop.h"
#include "Log.h"

#include <stdio.h>

using namespace blink;

const size_t kFramLen = 2 * sizeof(int64_t);

void serverConnectionCallback(const TcpConnectionPtr& connection)
{
    LOG_TRACE << connection->name() << " " << connection->peerAddress().toIpPort()
              << " - > " << connection->localAddress().toIpPort()
              << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        connection->setTcpNoDelay(true);
    }
}

void serverMessageCallback(const TcpConnectionPtr& connection,
                           Buffer* buf,
                           Timestamp receive_time)
{
    int64_t message[2];
    while (buf->readableSize() >= kFramLen)
    {
        memcpy(message, buf->peek(), kFramLen);
        buf->reset(kFramLen);
        message[1] = receive_time.microSecondsSinceEpoch();
        connection->send(message, sizeof(message));
    }
}

void runServer(uint16_t port)
{
    EventLoop loop;
    TcpServer server(&loop, InetAddress(port), "ClockServer");
    server.setConnectionCallback(serverConnectionCallback);
    server.setMessageCallback(serverMessageCallback);
    server.start();
    loop.loop();
}

TcpConnectionPtr client_connection;

void clientConnectionCallback(const TcpConnectionPtr& connection)
{
    LOG_TRACE << connection->name() << " " << connection->localAddress().toIpPort()
              << " - > " << connection->peerAddress().toIpPort()
              << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        client_connection = connection;
        connection->setTcpNoDelay(true);
    }
    else
    {
        client_connection.reset();
    }
}

void clientMessageCallback(const TcpConnectionPtr& connection,
                           Buffer* buf,
                           Timestamp receive_time)
{
    int64_t message[2];
    while (buf->readableSize() >= kFramLen)
    {
        memcpy(message, buf->peek(), kFramLen);
        buf->reset(kFramLen);
        int64_t send = message[0];
        int64_t their = message[1];
        int64_t back = receive_time.microSecondsSinceEpoch();
        int64_t mine = (back + send) / 2;
        LOG_INFO << "round trip " << back - send
                 << " clock error " << their - mine;
    }
}

void sendMyTime()
{
    if (client_connection)
    {
        int64_t message[2] = {0, 0};
        message[0] = Timestamp::now().microSecondsSinceEpoch();
        client_connection->send(message, sizeof(message));
    }
}

void runClient(const char* ip, uint16_t port)
{
    EventLoop loop;
    TcpClient client(&loop, InetAddress(ip, port), "ClockClient");
    client.enableRetry();
    client.setConnectionCallback(clientConnectionCallback);
    client.setMessageCallback(clientMessageCallback);
    client.connect();
    loop.runEvery(0.2, sendMyTime);
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
