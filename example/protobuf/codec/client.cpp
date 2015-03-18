#include <example/protobuf/codec/dispatcher.h>
#include <example/protobuf/codec/query.pb.h>
#include <example/protobuf/codec/codec.h>

#include <blink/TcpClient.h>
#include <blink/EventLoop.h>
#include <blink/MutexLock.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

typedef boost::shared_ptr<Empty> EmptyPtr;
typedef boost::shared_ptr<Answer> AnswerPtr;

google::protobuf::Message* g_MessageToSend = NULL;

class QueryClient : Nocopyable
{
public:
    QueryClient(EventLoop* loop,
                const InetAddress& server_addr,
                const string& name)
        : loop_(loop),
          client_(loop, server_addr, name),
          dispatcher_(boost::bind(&QueryClient::onUnknownMessage, this, _1, _2, _3)),
          codec_(boost::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
    {
        dispatcher_.registerMessageCallback<Answer>(boost::bind(&QueryClient::onAnswer, this, _1, _2, _3));
        dispatcher_.registerMessageCallback<Empty>(boost::bind(&QueryClient::onEmpty, this, _1, _2, _3));
        client_.setConnectionCallback(boost::bind(&QueryClient::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&ProtobufCodec::onMessage, &codec_, _1, _2, _3));
    }

    void connect()
    {
        client_.connect();
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_INFO << connection->localAddress().toIpPort() << " -> "
                 << connection->peerAddress().toIpPort() << " is "
                 << (connection->connected() ? "UP" : "DOWN");
        if (connection->connected())
        {
            codec_.send(connection, *g_MessageToSend);
        }
        else
        {
            loop_->quit();
        }
    }

    void onUnknownMessage(const TcpConnectionPtr& connection,
                          const MessagePtr& message,
                          Timestamp receive_time)
    {
        LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
    }

    void onAnswer(const TcpConnectionPtr& connection,
                  const AnswerPtr& message,
                  Timestamp receive_time)
    {
        LOG_INFO << "onAnswer:\n" << message->GetTypeName() << message->DebugString();
    }

    void onEmpty(const TcpConnectionPtr& connection,
                 const EmptyPtr& message,
                 Timestamp receive_time)
    {
        LOG_INFO << "onEmpty: " << message->GetTypeName();
    }

    EventLoop*          loop_;
    TcpClient           client_;
    ProtobufDispatcher  dispatcher_;
    ProtobufCodec       codec_;
};

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <ipaddr> <port> [q|e]\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid();
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress server_addr(argv[1], port);
    Query query;
    query.set_id(1);
    query.set_questioner("Li Yuan");
    query.add_question("Go?");
    Empty empty;
    if (argc == 4 && argv[3][0] == 'e')
    {
        g_MessageToSend = &empty;
    }
    else
    {
        g_MessageToSend = &query;
    }
    QueryClient client(&loop, server_addr, "QueryClient");
    client.connect();
    loop.loop();
    return 0;
}
