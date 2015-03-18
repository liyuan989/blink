#include <example/protobuf/codec/query.pb.h>
#include <example/protobuf/codec/codec.h>

#include <blink/Endian.h>

#include <zlib.h>

#include <stdio.h>

using namespace blink;

void print(const Buffer& buf)
{
    printf("encoding to %zd bytes\n", buf.readableSize());
    for (size_t i = 0; i < buf.readableSize(); ++i)
    {
        unsigned char ch = static_cast<unsigned char>(buf.peek()[i]);
        printf("%2zd 0x%02x %c\n", i, ch, isgraph(ch) ? ch : ' ');
    }
}

void testQerry()
{
    Query query;
    query.set_id(1);
    query.set_questioner("Li Yuan");
    query.add_question("go?");

    Buffer buf;
    ProtobufCodec::fillEmptyBuffer(&buf, query);
    print(buf);

    const int32_t len = buf.readInt32();
    assert(len == static_cast<int32_t>(buf.readableSize()));

    ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
    MessagePtr message = ProtobufCodec::parse(buf.peek(), len, &error_code);
    assert(error_code == ProtobufCodec::kNoError);
    assert(message != NULL);
    message->PrintDebugString();
    assert(message->DebugString() == query.DebugString());

    boost::shared_ptr<Query> new_query = down_pointer_cast<Query>(message);
    assert(new_query != NULL);
}

void testAnswer()
{
    Answer answer;
    answer.set_id(1);
    answer.set_questioner("Li Yuan");
    answer.set_answer("liyuanlife.com");
    answer.add_solution("go!");
    answer.add_solution("win!");

    Buffer buf;
    ProtobufCodec::fillEmptyBuffer(&buf, answer);
    print(buf);

    const int32_t len = buf.readInt32();
    assert(len == static_cast<int32_t>(buf.readableSize()));

    ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
    MessagePtr message = ProtobufCodec::parse(buf.peek(), len, &error_code);
    assert(error_code == ProtobufCodec::kNoError);
    assert(message != NULL);
    message->PrintDebugString();
    assert(message->DebugString() == answer.DebugString());

    boost::shared_ptr<Answer> new_answer = down_pointer_cast<Answer>(message);
    assert(new_answer != NULL);
}

void testEmpty()
{
    Empty empty;

    Buffer buf;
    ProtobufCodec::fillEmptyBuffer(&buf, empty);
    print(buf);

    const int32_t len = buf.readInt32();
    assert(len == static_cast<int32_t>(buf.readableSize()));

    ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
    MessagePtr message = ProtobufCodec::parse(buf.peek(), len, &error_code);
    assert(message != NULL);
    message->PrintDebugString();
    assert(message->DebugString() == empty.DebugString());
}

void redoChecksum(string& data, int len)
{
    int32_t checksum = sockets::hostToNetwork32(static_cast<int32_t>(
        adler32(1,
                reinterpret_cast<const Bytef*>(data.c_str()),
                static_cast<uInt>(len - 4))));
    data[len - 4] = reinterpret_cast<const char*>(&checksum)[0];
    data[len - 3] = reinterpret_cast<const char*>(&checksum)[1];
    data[len - 2] = reinterpret_cast<const char*>(&checksum)[2];
    data[len - 1] = reinterpret_cast<const char*>(&checksum)[3];
}

void testBadBuffer()
{
    Empty empty;
    empty.set_id(43);

    Buffer buf;
    ProtobufCodec::fillEmptyBuffer(&buf, empty);
    // print(buf);

    const int32_t len = buf.readInt32();
    assert(len == static_cast<int32_t>(buf.readableSize()));

    {
        string data(buf.peek(), len);
        ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
        MessagePtr message = ProtobufCodec::parse(data.c_str(), len - 1, &error_code);
        assert(message == NULL);
        assert(error_code == ProtobufCodec::kChecksumError);
    }

    {
        string data(buf.peek(), len);
        ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
        ++data[len - 1];
        MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &error_code);
        assert(message == NULL);
        assert(error_code == ProtobufCodec::kChecksumError);
    }

    {
        string data(buf.peek(), len);
        ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
        ++data[0];
        MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &error_code);
        assert(message == NULL);
        assert(error_code == ProtobufCodec::kChecksumError);
    }

    {
        string data(buf.peek(), len);
        ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
        data[3] = 0;
        redoChecksum(data, len);
        MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &error_code);
        assert(message == NULL);
        assert(error_code == ProtobufCodec::kInvalidNameLen);
    }

    {
        string data(buf.peek(), len);
        ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
        data[3] = 100;
        redoChecksum(data, len);
        MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &error_code);
        assert(message == NULL);
        assert(error_code == ProtobufCodec::kInvalidNameLen);
    }

    {
        string data(buf.peek(), len);
        ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
        --data[3];
        redoChecksum(data, len);
        MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &error_code);
        assert(message == NULL);
        assert(error_code == ProtobufCodec::kUnknownMessageType);
    }

    {
        string data(buf.peek(), len);
        ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
        data[4] = 'M';
        redoChecksum(data, len);
        MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &error_code);
        assert(message == NULL);
        assert(error_code == ProtobufCodec::kUnknownMessageType);
    }

    {
        // FIXME: reproduce parse errpr
        string data(buf.peek(), len);
        ProtobufCodec::ErrorCode error_code = ProtobufCodec::kNoError;
        redoChecksum(data, len);
        MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &error_code);
        //assert(message == NULL);
        //assert(error_code == ProtobufCodec::kParseError);
    }
}

int g_count = 0;

void onMessage(const TcpConnectionPtr& connection,
               const MessagePtr& message,
               Timestamp receive_time)
{
    ++g_count;
}

void testOnMessage()
{
    Query query;
    query.set_id(1);
    query.set_questioner("Li Yuan");
    query.add_question("go?");

    Buffer buf1;
    ProtobufCodec::fillEmptyBuffer(&buf1, query);

    Empty empty;
    empty.set_id(43);
    empty.set_id(9998);

    Buffer buf2;
    ProtobufCodec::fillEmptyBuffer(&buf2, empty);

    size_t total_len = buf1.readableSize() + buf2.readableSize();

    Buffer all;
    all.append(buf1.peek(), buf1.readableSize());
    all.append(buf2.peek(), buf2.readableSize());
    assert(all.readableSize() == total_len);

    TcpConnectionPtr connection;
    Timestamp t;
    ProtobufCodec codec(onMessage);

    for (size_t len = 0; len <= total_len; ++len)
    {
        Buffer input;
        input.append(all.peek(), len);

        g_count = 0;
        codec.onMessage(connection, &input, t);
        int expected = len < buf1.readableSize() ? 0 : 1;
        if (len == total_len)
        {
            expected = 2;
        }
        assert(g_count == expected);
        (void)expected;
        //printf("%2zd %d\n", len, g_count);

        input.append(all.peek() + len, total_len - len);
        codec.onMessage(connection, &input, t);
        assert(g_count == 2);
    }
}

int main(int argc, char* argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    testQerry();
    printf("--------------\n");
    testAnswer();
    printf("--------------\n");
    testEmpty();
    printf("--------------\n");
    testBadBuffer();
    printf("--------------\n");
    testOnMessage();
    printf("--------------\n");
    printf("all pass!\n");

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
