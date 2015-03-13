#undef NDEBUG
#include <blink/protorpc/RpcCodec.h>
#include <blink/protorpc/rpc.pb.h>
#include <blink/protobuf/ProtobufCodecLite.h>
#include <blink/Buffer.h>

#include <stdio.h>

using namespace blink;

void rpcMessageCallback(const TcpConnectionPtr&,
                        const RpcMessagePtr&,
                        Timestamp)
{
}

MessagePtr g_messagePtr;

void messageCallback(const TcpConnectionPtr&,
                     const MessagePtr& message,
                     Timestamp)
{
    g_messagePtr = message;
}

void print(const Buffer& buf)
{
    printf("encoded to %zd bytes\n", buf.readableSize());
    for (size_t i = 0; i < buf.readableSize(); ++i)
    {
        unsigned char ch = static_cast<unsigned char>(buf.peek()[i]);
        printf("%2zd:  0x%02x  %c\n", i, ch, isgraph(ch) ? ch : ' ');
    }
}

char rpctag[] = "RPC0";

int main(int argc, char* argv[])
{
    RpcMessage message;
    message.set_type(REQUEST);
    message.set_id(2);
    char wire[] = "\0\0\0\x13" "RPC0" "\x08\x01\x11\x02\0\0\0\0\0\0\0" "\x0f\xef\x01\x32";
    string expected(wire, sizeof(wire) - 1);
    string s1;
    string s2;
    Buffer buf1;
    Buffer buf2;

    {
        RpcCodec codec(rpcMessageCallback);
        codec.fillEmptyBuffer(&buf1, message);
        print(buf1);
        s1 = buf1.toStringPiece().asString();
    }

    {
        ProtobufCodecLite codec(&RpcMessage::default_instance(), "RPC0", messageCallback);
        codec.fillEmptyBuffer(&buf2, message);
        print(buf2);
        s2 = buf2.toStringPiece().asString();
        codec.onMessage(TcpConnectionPtr(), &buf1, Timestamp::now());
        assert(g_messagePtr);
        assert(g_messagePtr->DebugString() == message.DebugString());
        g_messagePtr.reset();
    }

    assert(s1 == s2);
    assert(s1 == expected);
    assert(s2 == expected);

    {
        Buffer buf;
        ProtobufCodecLite codec(&RpcMessage::default_instance(), "XYZ", messageCallback);
        codec.fillEmptyBuffer(&buf, message);
        print(buf);
        s2 = buf.toStringPiece().asString();
        codec.onMessage(TcpConnectionPtr(), &buf, Timestamp::now());
        assert(g_messagePtr);
        assert(g_messagePtr->DebugString() == message.DebugString());
        printf("%s\n", message.DebugString().c_str());
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
