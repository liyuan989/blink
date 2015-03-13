#include <example/hub/PubSubClient.h>
#include <example/hub/Codec.h>

#include <boost/bind.hpp>

using namespace blink;

PubSubClient::PubSubClient(EventLoop* loop,
                           const InetAddress& server_addr,
                           const string& name)
    : client_(loop, server_addr, name)
{
    // FIXME: destructor is not thread safd.
    client_.setConnectionCallback(boost::bind(&PubSubClient::onConnection, this, _1));
    client_.setMessageCallback(boost::bind(&PubSubClient::onMessage, this, _1, _2, _3));
}

void PubSubClient::start()
{
    client_.connect();
}

void PubSubClient::stop()
{
    client_.disconnect();
}

bool PubSubClient::connected()
{
    return connection_ && connection_->connected();
}

bool PubSubClient::subscribe(const string& topic, const SubscribeCallback& cb)
{
    string message = "sub " + topic + "\r\n";
    subscribe_callback_ = cb;
    return send(message);
}

void PubSubClient::unsubscribe(const string& topic)
{
    string message = "unsub " + topic + "\r\n";
    send(message);
}

bool PubSubClient::publish(const string& topic, const string& content)
{
    string message = "pub " + topic + "\r\n" + content + "\r\n";
    return send(message);
}

void PubSubClient::onConnection(const TcpConnectionPtr& connection)
{
    if (connection->connected())
    {
        connection_ = connection;  // FIXME: re-sub.
    }
    else
    {
        connection_.reset();
    }
    if (connection_callback_)
    {
        connection_callback_(this);
    }
}

void PubSubClient::onMessage(const TcpConnectionPtr& connection,
                             Buffer* buf,
                             Timestamp receive_time)
{
    ParseResult result = kSuccess;
    while (result == kSuccess)
    {
        string command;
        string topic;
        string content;
        result = parseMessage(buf, &command, &topic, &content);
        if (result == kSuccess)
        {
            if (command == "pub" && subscribe_callback_)
            {
                subscribe_callback_(topic, content, receive_time);
            }
        }
        else if (result == kError)
        {
            connection->shutdown();
        }
    }
}

bool PubSubClient::send(const string& message)
{
    bool succeed = false;
    if (connection_ && connection_->connected())
    {
        connection_->send(message);
        succeed = true;
    }
    return succeed;
}
