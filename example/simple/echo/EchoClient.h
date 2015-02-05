#ifndef __EXAMPLE_ECHOCLIENT_H__
#define __EXAMPLE_ECHOCLIENT_H__

#include "Nocopyable.h"
#include "TcpClient.h"
#include "EventLoop.h"
#include "InetAddress.h"

class EchoClient : blink::Nocopyable
{
public:
    EchoClient(blink::EventLoop* loop,
               const blink::InetAddress& server_addr,
               const blink::string& name);

    void connect();

private:
    void onConnection(const blink::TcpConnectionPtr& connection);

    void onMessage(const blink::TcpConnectionPtr& connection,
                   blink::Buffer* buf,
                   blink::Timestamp time);

    blink::EventLoop*  loop_;
    blink::TcpClient   client_;
};

#endif
