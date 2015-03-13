#ifndef __EXAMPLE_HUB_PUBSUB_H__
#define __EXAMPLE_HUB_PUBSUB_H__

#include <blink/TcpClient.h>
#include <blink/Nocopyable.h>

// FIXME: destructor is not thread safe.
class PubSubClient : blink::Nocopyable
{
public:
    typedef boost::function<void (PubSubClient*)> ConnectionCallback;
    typedef boost::function<void (const blink::string& topic,
                                  const blink::string& content,
                                  blink::Timestamp)> SubscribeCallback;

    PubSubClient(blink::EventLoop* loop,
                 const blink::InetAddress& server_addr,
                 const blink::string& name);

    void start();
    void stop();
    bool connected();
    bool subscribe(const blink::string& topic, const SubscribeCallback& cb);
    void unsubscribe(const blink::string& topic);
    bool publish(const blink::string& topic, const blink::string& content);

    void setConnectionCallback(const ConnectionCallback& cb)
    {
        connection_callback_ = cb;
    }

private:
    void onConnection(const blink::TcpConnectionPtr& connection);
    void onMessage(const blink::TcpConnectionPtr& connection,
                   blink::Buffer* buf,
                   blink::Timestamp receive_time);
    bool send(const blink::string& message);

    blink::TcpClient         client_;
    blink::TcpConnectionPtr  connection_;
    ConnectionCallback       connection_callback_;
    SubscribeCallback        subscribe_callback_;
};

#endif
