#ifndef __EXAMPLE_HUB_PUBSUB_H__
#define __EXAMPLE_HUB_PUBSUB_H__

#include "TcpClient.h"
#include "Nocopyable.h"

// FIXME: destructor is not thread safe.
class PubSubClient : blink::Nocopyable
{
public:
    typedef boost::function<void (PubSubClient*)> ConnectionCallback;
    typedef boost::function<void (const std::string& topic,
                                  const std::string& content,
                                  blink::Timestamp)> SubscribeCallback;

    PubSubClient(blink::EventLoop* loop,
                 const blink::InetAddress& server_addr,
                 const std::string& name);

    void start();
    void stop();
    bool connected();
    bool subscribe(const std::string& topic, const SubscribeCallback& cb);
    void unsubscribe(const std::string& topic);
    bool publish(const std::string& topic, const std::string& content);

    void setConnectionCallback(const ConnectionCallback& cb)
    {
        connection_callback_ = cb;
    }

private:
    void onConnection(const blink::TcpConnectionPtr& connection);
    void onMessage(const blink::TcpConnectionPtr& connection,
                   blink::Buffer* buf,
                   blink::Timestamp receive_time);
    bool send(const std::string& message);

    blink::TcpClient         client_;
    blink::TcpConnectionPtr  connection_;
    ConnectionCallback       connection_callback_;
    SubscribeCallback        subscribe_callback_;
};

#endif
