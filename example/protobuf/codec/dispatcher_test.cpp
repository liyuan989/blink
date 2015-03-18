#include <example/protobuf/codec/dispatcher.h>
#include <example/protobuf/codec/query.pb.h>

#include <iostream>

using namespace blink;

typedef boost::shared_ptr<Query> QueryPtr;
typedef boost::shared_ptr<Answer> AnswerPtr;

void test_down_pointer_cast()
{
    boost::shared_ptr<google::protobuf::Message> message(new Query);
    boost::shared_ptr<Query> query(down_pointer_cast<Query>(message));
    assert(message && query);
    if (!query)
    {
        abort();
    }
}

void onQuery(const TcpConnectionPtr& connection,
             const QueryPtr& message,
             Timestamp)
{
    std::cout << "onQuery: " << message->GetTypeName() << std::endl;
}

void onAnswer(const TcpConnectionPtr& connection,
              const AnswerPtr& message,
              Timestamp)
{
    std::cout << "onAnwser: " <<message->GetTypeName() << std::endl;
}

void onUnknownMessageType(const TcpConnectionPtr& connection,
                        const MessagePtr& message,
                        Timestamp)
{
    std::cout << "onknownMessageType: " << message->GetTypeName() << std::endl;
}

int main(int argc, char* argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    test_down_pointer_cast();
    ProtobufDispatcher dispatcher(onUnknownMessageType);
    dispatcher.registerMessageCallback<Query>(onQuery);
    dispatcher.registerMessageCallback<Answer>(onAnswer);

    TcpConnectionPtr connection;
    Timestamp t;

    boost::shared_ptr<Query> query(new Query);
    boost::shared_ptr<Answer> answer(new Answer);
    boost::shared_ptr<Empty> empty(new Empty);
    dispatcher.onProtobufMessage(connection, query, t);
    dispatcher.onProtobufMessage(connection, answer, t);
    dispatcher.onProtobufMessage(connection, empty, t);

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
