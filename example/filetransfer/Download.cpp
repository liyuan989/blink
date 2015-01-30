#include "EventLoop.h"
#include "TcpServer.h"
#include "Log.h"

#include <unistd.h>

#include <stdio.h>

using namespace blink;

const char* g_filename = NULL;

std::string readFile(const char* filename)     // FIXME: use readFile() in FileTool.h
{
    std::string content;
    FILE* fp = ::fopen(filename, "rb");
    if (fp)
    {
        const int kBufferSize = 1024 * 1024;   // FIXME: inefficient
        char io_buf[kBufferSize];
        setbuffer(fp, io_buf, sizeof(io_buf));

        char buf[kBufferSize];

        size_t nread = 0;
        while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0)
        {
            content.append(buf, nread);
        }
        fclose(fp);
    }
    return content;
}

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
        connection->setHighWaterMarkCallback(onHighWaterMark, 64 * 1024);
        std::string file_content = readFile(g_filename);
        connection->send(file_content);
        connection->shutdown();
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
    server.start();
    loop.loop();
    return 0;
}
