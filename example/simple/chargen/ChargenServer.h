#ifndef __EXAMPLE_CHARGENSERVER_H__
#define __EXAMPLE_CHARGENSERVER_H__

#include "Nocopyable.h"
#include "TcpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"

#include <string>

class ChargenServer : blink::Nocopyable
{
public:
    ChargenServer(blink::EventLoop* loop,
                  const blink::InetAddress& server_addr,
                  const std::string& name,
                  bool print = false);

    void start();

private:
    void onConnection(const blink::TcpConnectionPtr& connection);
    void onMessage(const blink::TcpConnectionPtr& connection,
                   blink::Buffer* buf,
                   blink::Timestamp time);
    void onWriteComplete(const blink::TcpConnectionPtr& connection);
    void printThroughput();

    blink::EventLoop*   loop_;
    blink::TcpServer    server_;
    std::string         message_;
    int64_t             transferred_;
    blink::Timestamp    start_time_;
};

#endif
