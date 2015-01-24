#include "Codec.h"

#include "TcpServer.h"
#include "EventLoop.h"
#include "Nocopyable.h"
#include "Copyable.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <map>
#include <set>
#include <stdio.h>

using namespace blink;

typedef std::set<std::string> ConnectionSubscription;

class Topic : Copyable
{
public:
    Topic(const std::string& topic)
        : topic_(topic)
    {
    }

    void add(const TcpConnectionPtr& connection)
    {
        audiences_.insert(connection);
        if (last_pub_time_.valid())
        {
            connection->send(makeMessage());
        }
    }

    void remove(const TcpConnectionPtr& connection)
    {
        audiences_.erase(connection);
    }

    void publish(const std::string& content, Timestamp time)
    {
        content_ = content;
        last_pub_time_ = time;
        std::string message = makeMessage();
        for (std::set<TcpConnectionPtr>::iterator it = audiences_.begin();
             it != audiences_.end(); ++it)
        {
            (*it)->send(message);
        }
    }

private:
    std::string makeMessage()
    {
        return "pub " + topic_ + "\r\n" + content_ + "\r\n";
    }

    std::string                 topic_;
    std::string                 content_;
    Timestamp                   last_pub_time_;
    std::set<TcpConnectionPtr>  audiences_;
};

class PubSubServer : Nocopyable
{
public:
    PubSubServer(EventLoop* loop, const InetAddress& listen_addr)
        : loop_(loop), server_(loop, listen_addr, "PubSubServer")
    {
        server_.setConnectionCallback(boost::bind(&PubSubServer::onConnection, this, _1));
        server_.setMessageCallback(boost::bind(&PubSubServer::onMessage, this, _1, _2, _3));
        loop_->runAfter(1.0, boost::bind(&PubSubServer::timePublish, this));
    }

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        if (connection->connected())
        {
            connection->setContext(ConnectionSubscription());
        }
        else
        {
            const ConnectionSubscription& connection_sub
                = boost::any_cast<const ConnectionSubscription&>(connection->getContext());
            // subtle: diUnsubscribe will erase *it , so increase before calling.
            for (ConnectionSubscription::iterator it = connection_sub.begin();
                 it != connection_sub.end(); ++it)
            {
                doUnsubscribe(connection, *it);
            }
        }
    }

    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time)
    {
        ParseResult result = kSuccess;
        while (result == kSuccess)
        {
            std::string command;
            std::string topic;
            std::string content;
            result = parseMessage(buf, &command, &topic, &content);
            if (result == kSuccess)
            {
                if (command == "pub")
                {
                    doPublish(connection->name(), topic, content, receive_time);
                }
                else if (command == "sub")
                {
                    LOG_INFO << connection->name() << " subscribes " << topic;
                    doSubscribe(connection, topic);
                }
                else if (command == "unsub")
                {
                    doUnsubscribe(connection, topic);
                }
                else
                {
                    connection->shutdown();
                    result = kError;
                }
            }
            else if (result == kError)
            {
                connection->shutdown();
            }
        }
    }

    void timePublish()
    {
        Timestamp now = Timestamp::now();
        doPublish("internal", "utc_time", now.toFormattedString(), now);
    }

    void doSubscribe(const TcpConnectionPtr& connection, const std::string& topic)
    {
        ConnectionSubscription* connection_sub
            = boost::any_cast<ConnectionSubscription>(connection->getMutableContext());
        connection_sub->insert(topic);
        getTopic(topic).add(connection);
    }

    void doUnsubscribe(const TcpConnectionPtr& connection, const std::string& topic)
    {
        LOG_INFO << connection->name() << " unsubscribes " << topic;
        getTopic(topic).remove(connection);
        // topic could be the one to be destroyed, so don't use it after erasing.
        ConnectionSubscription* connection_sub
            = boost::any_cast<ConnectionSubscription>(connection->getMutableContext());
        connection_sub->erase(topic);
    }

    void doPublish(const std::string& source,
                   const std::string& topic,
                   const std::string& content,
                   Timestamp time)
    {
        getTopic(topic).publish(content, time);
    }

    Topic& getTopic(const std::string& topic)
    {
        std::map<std::string, Topic>::iterator it = topics_.find(topic);
        if (it == topics_.end())
        {
            it = topics_.insert(std::make_pair(topic, Topic(topic))).first;
        }
        return it->second;
    }

    EventLoop*                    loop_;
    TcpServer                     server_;
    std::map<std::string, Topic>  topics_;
};

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    EventLoop loop;
    InetAddress listen_addr(port);
    PubSubServer server(&loop, listen_addr);
    server.start();
    loop.loop();
    return 0;
}
