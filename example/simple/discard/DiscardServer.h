#ifndef __EXAMPLE_DISCARDSERVER_H__
#define __EXAMPLE_DISCARDSERVER_H__

#include <blink/Nocopyable.h>
#include <blink/TcpServer.h>
#include <blink/EventLoop.h>
#include <blink/InetAddress.h>

class DiscardServer : blink::Nocopyable
{
public:
    DiscardServer(blink::EventLoop* loop,
                  const blink::InetAddress& server_addr,
                  const blink::string& name);

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
