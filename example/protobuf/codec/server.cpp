#include <example/protobuf/codec/codec.h>
#include <example/protobuf/codec/dispatcher.h>
#include <example/protobuf/codec/query.pb.h>

#include <blink/EventLoop.h>
#include <blink/TcpServer.h>
#include <blink/MutexLock.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

typedef boost::shared_ptr<Query> QueryPtr;
typedef boost::shared_ptr<Answer> AnswerPtr;

class QueryServer : Nocopyable
{
public:
    QueryServer(EventLoop* loop,
                const InetAddress& server_addr,
                const string& name)
        : server_(loop, server_addr, name),
          dispatcher_(boost::bind(&QueryServer::onUnknownMessage, this, _1, _2, _3)),
          codec_(boost::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
    {
        dispatcher_.registerMessageCallback<Query>(boost::bind(&QueryServer::onQuerry, this, _1, _2, _3));
        dispatcher_.registerMessageCallback<Answer>(boost::bind(&QueryServer::onAnswer, this, _1, _2, _3));
        server_.setConnectionCallback(boost::bind(&QueryServer::onConnection, this, _1));
        server_.setMessageCallback(boost::bind(&ProtobufCodec::onMessage, &codec_, _1, _2, _3));
    }

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_INFO << connection->peerAddress().toIpPort() << " -> "
                 << connection->localAddress().toIpPort() << " is "
                 << (connection->connected() ? "UP" : "DOWN");
    }

    void onUnknownMessage(const TcpConnectionPtr& connection,
                          const MessagePtr& message,
                          Timestamp receive_time)
    {
        LOG_INFO << " onUnknownMessage: " << message->GetTypeName();
        connection->shutdown();
    }

    void onQuerry(const TcpConnectionPtr& connection,
                  const QueryPtr& message,
                  Timestamp receive_time)
    {
        LOG_INFO << "onQuerry:\n" << message->GetTypeName() << message->DebugString();
        Answer answer;
        answer.set_id(1);
        answer.set_questioner("Li Yuan");
        answer.set_answer("liyuanlife.com");
        answer.add_solution("run!");
        answer.add_solution("go!");
        answer.add_solution("win!");

        codec_.send(connection, answer);
        connection->shutdown();
    }

    void onAnswer(const TcpConnectionPtr& connection,
                  const AnswerPtr& message,
                  Timestamp receive_time)
    {
        LOG_INFO << "onAnswer: " << message->GetTypeName();
        connection->shutdown();
    }

    TcpServer           server_;
    ProtobufDispatcher  dispatcher_;
    ProtobufCodec       codec_;
};

int main(int argc, char* argv[])
{
    LOG_INFO << "pid = " << getpid();
    EventLoop loop;
    QueryServer server(&loop, InetAddress(9600), "QueryServer");
    server.start();
    loop.loop();
    return 0;
}
