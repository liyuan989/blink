#ifndef __EXAMPLE_PROTOBUF_CODEC_CODEC_H__
#define __EXAMPLE_PROTOBUF_CODEC_CODEC_H__

#include <blink/TcpConnection.h>
#include <blink/Nocopyable.h>
#include <blink/Buffer.h>

#include <google/protobuf/message.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

// struct ProtobufTransportFormat __attribute__ ((__packed__))
// {
//   int32_t  len;
//   int32_t  nameLen;
//   char     typeName[nameLen];
//   char     protobufData[len-nameLen-8];
//   int32_t  checkSum; // adler32 of nameLen, typeName and protobufData
// }

typedef boost::shared_ptr<google::protobuf::Message> MessagePtr;

class ProtobufCodec : blink::Nocopyable
{
public:
    enum ErrorCode
    {
        kNoError = 0,
        kInvalidLength,
        kChecksumError,
        kInvalidNameLen,
        kUnknownMessageType,
        kParseError,
    };

    typedef boost::function<void (const blink::TcpConnectionPtr&,
                                  const MessagePtr&,
                                  blink::Timestamp)> ProtobufMessageCallback;

    typedef boost::function<void (const blink::TcpConnectionPtr&,
                                  blink::Buffer*,
                                  blink::Timestamp,
                                  ErrorCode)> ErrorCallback;

    explicit ProtobufCodec(const ProtobufMessageCallback& message_callback)
        : message_callback_(message_callback),
          error_callback_(defaultErrorCallback)
    {
    }

    ProtobufCodec(const ProtobufMessageCallback& message_callback,
                  const ErrorCallback& error_callback)
        : message_callback_(message_callback),
          error_callback_(error_callback)
    {
    }

    void send(const blink::TcpConnectionPtr& connection,
              const google::protobuf::Message& message)
    {
        blink::Buffer buf;
        fillEmptyBuffer(&buf, message);
        connection->send(&buf);
    }

    void onMessage(const blink::TcpConnectionPtr& connection,
                   blink::Buffer* buf,
                   blink::Timestamp receive_time);

    static const blink::string& errorCodeToString(ErrorCode error_code);
    static void fillEmptyBuffer(blink::Buffer* buf, const google::protobuf::Message& message);
    static google::protobuf::Message* createMessage(const std::string& type_name);
    static MessagePtr parse(const char* buf, int len, ErrorCode* error_code);

private:
    static void defaultErrorCallback(const blink::TcpConnectionPtr& connection,
                                     blink::Buffer* buf,
                                     blink::Timestamp receive_time,
                                     ErrorCode error_code);

    ProtobufMessageCallback  message_callback_;
    ErrorCallback            error_callback_;

    static const int kHeaderLen = sizeof(int32_t);
    static const int kMinMessageLen = 2 * kHeaderLen + 2;  // nameLen + typeName + checksum
    static const int kMaxMessageLen = 64 * 1024 * 1024;  // same as codec_stream.h kDefaultTotalBytesLimit
};

#endif
