#ifndef __EXAMPLE_DAYTIMESERVER_H__
#define __EXAMPLE_DAYTIMESERVER_H__

#include "Nocopyable.h"
#include "TcpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"

#include <string>

class DaytimeServer : blink::Nocopyable
{
public:
    DaytimeServer(blink::EventLoop* loop,
                  const blink::InetAddress& server_addr,
                  const std::string& name);

    void start();

private:
    void onConnection(const blink::TcpConnectionPtr& connection);
    void onMessage(const blink::TcpConnectionPtr& connection,
                   blink::Buffer* buf,
                   blink::Timestamp time);

    blink::EventLoop*  loop_;
    blink::TcpServer   server_;
};

#endif
