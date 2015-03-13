#ifndef __BLINK_PROTOBUF_PROTOBUFCODECLITE_H__
#define __BLINK_PROTOBUF_PROTOBUFCODECLITE_H__

#include <blink/Nocopyable.h>
#include <blink/StringPiece.h>
#include <blink/Timestamp.h>
#include <blink/Callbacks.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#ifndef NDEBUG
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_of.hpp>
#endif

namespace google
{

namespace protobuf
{

class Message;

}  // namespace protobuf

}  // namespace google

namespace blink
{

class Buffer;
typedef boost::shared_ptr<google::protobuf::Message> MessagePtr;

// wire format
//
// Field     Length  Content
//
// size      4-byte  M+N+4
// tag       M-byte  could be "RPC0", etc.
// payload   N-byte
// checksum  4-byte  adler32 of tag+payload

class ProtobufCodecLite : Nocopyable
{
public:
    static const int kHeaderLen = sizeof(int32_t);
    static const int kChecksumLen = sizeof(int32_t);
    static const int kMaxMessageLen = 64 * 1024 * 1024;  // same as codec_stream.h kDefaultTotalBytesLimit

    enum ErrorCode
    {
        kNoError = 0,
        kInvalidLength,
        kChecksumError,
        kInvalidNameLen,
        kUnknownMessageType,
        kParseError,
    };

    // return false to stop parsing protobuf message
    typedef boost::function<bool (const TcpConnectionPtr&,
                                  StringPiece,
                                  Timestamp)> RawMessageCallback;

    typedef boost::function<void (const TcpConnectionPtr&,
                                  const MessagePtr&,
                                  Timestamp)> ProtobufMessageCallback;

    typedef boost::function<void (const TcpConnectionPtr&,
                                  Buffer*,
                                  Timestamp,
                                  ErrorCode)> ErrorCallback;

    ProtobufCodecLite(const ::google::protobuf::Message* proto_type,
                      StringPiece tag_arg,
                      const ProtobufMessageCallback& message_callback,
                      const RawMessageCallback& raw_callback = RawMessageCallback(),
                      const ErrorCallback& error_callback = defaultErrorCallback)
        : proto_type_(proto_type),
          tag_(tag_arg.asString()),
          message_callback_(message_callback),
          raw_callback_(raw_callback),
          error_callback_(error_callback),
          kMinMessageLen(tag_arg.size() + kChecksumLen)
    {
    }

    virtual ~ProtobufCodecLite()
    {
    }

    const string& tag() const
    {
        return tag_;
    }

    void send(const TcpConnectionPtr& connection,
              const ::google::protobuf::Message& message);
    void onMessage(const TcpConnectionPtr& connection,
                    Buffer* buf,
                    Timestamp receive_time);
    virtual bool parseFromBuffer(StringPiece buf, ::google::protobuf::Message* message);
    virtual int serializeToBuffer(const ::google::protobuf::Message& message, Buffer* buf);

    // public for unit test
    ErrorCode parse(const char* buf, int len, ::google::protobuf::Message* message);
    void fillEmptyBuffer(Buffer* buf, const ::google::protobuf::Message& message);

    static const string& errorCodeToString(ErrorCode error_code);
    static int32_t checksum(const void* buf, int len);
    static bool validateChecksum(const char* buf, int len);
    static int32_t asInt32(const char* buf);
    static void defaultErrorCallback(const TcpConnectionPtr& connection,
                                     Buffer* buf,
                                     Timestamp receive_time,
                                     ErrorCode error_code);

private:
    const ::google::protobuf::Message*  proto_type_;
    const string                        tag_;
    ProtobufMessageCallback             message_callback_;
    RawMessageCallback                  raw_callback_;
    ErrorCallback                       error_callback_;
    const int                           kMinMessageLen;
};

// TAG must be a variable with external linkage, not a string literal
template<typename MSG, const char* TAG, typename CODEC = ProtobufCodecLite>
class ProtobufCodecLiteT
{
#ifndef NDEBUG
    BOOST_STATIC_ASSERT(boost::is_base_of<ProtobufCodecLite, CODEC>::value);
#endif

public:
    typedef boost::shared_ptr<MSG> ConcreteMessagePtr;
    typedef boost::function<void (const TcpConnectionPtr&,
                                  const ConcreteMessagePtr&,
                                  Timestamp)> ProtobufMessageCallback;
    typedef ProtobufCodecLite::RawMessageCallback RawMessageCallback;
    typedef ProtobufCodecLite::ErrorCallback ErrorCallback;

    explicit ProtobufCodecLiteT(const ProtobufMessageCallback& message_callback,
                                const RawMessageCallback& raw_callback = RawMessageCallback(),
                                const ErrorCallback error_callback = ProtobufCodecLite::defaultErrorCallback)
        : message_callback_(message_callback),
          codec_(&MSG::default_instance(),
                 TAG,
                 boost::bind(&ProtobufCodecLiteT::onRpcMessage, this, _1, _2, _3),
                 raw_callback,
                 error_callback)
    {
    }

    const string& tag() const
    {
        return codec_.tag();
    }

    void send(const TcpConnectionPtr& connection,
              const MSG& message)
    {
        codec_.send(connection, message);
    }

    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time)
    {
        codec_.onMessage(connection, buf, receive_time);
    }

    // internal
    void onRpcMessage(const TcpConnectionPtr& connection,
                      const MessagePtr& message,
                      Timestamp receive_time)
    {
        message_callback_(connection, down_pointer_cast<MSG>(message), receive_time);
    }

    void fillEmptyBuffer(Buffer* buf, const MSG& message)
    {
        codec_.fillEmptyBuffer(buf, message);
    }

private:
    ProtobufMessageCallback  message_callback_;
    CODEC                    codec_;
};

}  // namespace blink

#endif
