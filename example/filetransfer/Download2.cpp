#include "EventLoop.h"
#include "TcpServer.h"
#include "Log.h"

#include <unistd.h>

#include <stdio.h>

using namespace blink;

const int kBuffSize = 64 * 1024;
const char* g_filename = NULL;

void onHighWaterMark(const TcpConnectionPtr& connection, size_t len)
{
    LOG_INFO << "HighWaterMark " << len;
}

void onConnection(const TcpConnectionPtr& connection)
{
    LOG_INFO << "FileServer - " << connection->peerAddress().toIpPort() << " -> "
             << connection->localAddress().toIpPort() << " is "
             << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        LOG_INFO << "FileServer - Sending file " << g_filename
                 << " to " << connection->peerAddress().toIpPort();
        connection->setHighWaterMarkCallback(onHighWaterMark, kBuffSize + 1);
        FILE* fp = fopen(g_filename, "rb");
        if (fp)
        {
            connection->setContext(fp);
            char buf[kBuffSize];
            size_t nread = fread(buf, 1, sizeof(buf), fp);
            connection->send(buf, nread);
        }
        else
        {
            connection->shutdown();
            LOG_INFO << "FileServer - no such file";
        }
    }
    else
    {
        if (!connection->getContext().empty())
        {
            FILE* fp = boost::any_cast<FILE*>(connection->getContext());
            if (fp)
            {
                fclose(fp);
            }
        }
    }
}

void onWriteComplete(const TcpConnectionPtr& connection)
{
    FILE* fp = boost::any_cast<FILE*>(connection->getContext());
    char buf[kBuffSize];
    size_t nread = fread(buf, 1, sizeof(buf), fp);
    if (nread > 0)
    {
        connection->send(buf, nread);
    }
    else
    {
        fclose(fp);
        fp = NULL;
        connection->setContext(fp);
        LOG_INFO << "FileServer - done";
    }
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    g_filename = argv[1];
    EventLoop loop;
    TcpServer server(&loop, InetAddress(9600), "FileServer");
    server.setConnectionCallback(onConnection);
    server.setWriteCompleteCallback(onWriteComplete);
    server.start();
    loop.loop();
    return 0;
}
