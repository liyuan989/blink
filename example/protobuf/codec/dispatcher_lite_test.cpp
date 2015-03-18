#include <example/protobuf/codec/dispatcher_lite.h>
#include <example/protobuf/codec/query.pb.h>

#include <iostream>

using namespace blink;

typedef boost::shared_ptr<Query> QueryPtr;
typedef boost::shared_ptr<Answer> AnswerPtr;

void onQuery(const TcpConnectionPtr& connection,
             const MessagePtr& message,
             Timestamp)
{
    std::cout << "onQuery: " << message->GetTypeName() << std::endl;
    boost::shared_ptr<Query> query = down_pointer_cast<Query>(message);
    assert(query != NULL);
}

void onAnswer(const TcpConnectionPtr& connection,
              const MessagePtr& message,
              Timestamp)
{
    std::cout << "onAnwser: " <<message->GetTypeName() << std::endl;
    boost::shared_ptr<Answer> answer = down_pointer_cast<Answer>(message);
    assert(answer != NULL);
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
    ProtobufDispatcherLite dispatcher(onUnknownMessageType);
    dispatcher.registerMessageCallback(Query::descriptor(), onQuery);
    dispatcher.registerMessageCallback(Answer::descriptor(), onAnswer);

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
