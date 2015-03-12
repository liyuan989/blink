#include "protobuf/ProtobufCodecLite.h"
#include "protorpc/google-inl.h"
#include "TcpConnection.h"
#include "Endian.h"
#include "Log.h"

#include <zlib.h>

#include <google/protobuf/message.h>

namespace blink
{

namespace
{

int protobufVersionCheck()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    return 0;
}

int dummy = protobufVersionCheck();
string kNoErrorStr = "NoError";
string kInvalidLengthStr = "InvalidLength";
string kChecksumErrorStr = "ChecksumError";
string kInvalidNameLenStr = "InvalidNameLen";
string kUnknownMessageTypeStr = "UnknownMessageType";
string kParseErrorStr = "ParseError";
string kUnknownErrorStr = "UnknownError";

}  // anonoymous namespace

const int ProtobufCodecLite::kHeaderLen;
const int ProtobufCodecLite::kChecksumLen;
const int ProtobufCodecLite::kMaxMessageLen;

void ProtobufCodecLite::send(const TcpConnectionPtr& connection,
                             const ::google::protobuf::Message& message)
{
    // FIXME: serialize to TcpConnection::outputBuffer()
    Buffer buf;
    fillEmptyBuffer(&buf, message);
    connection->send(&buf);
}

void ProtobufCodecLite::fillEmptyBuffer(Buffer* buf, const ::google::protobuf::Message& message)
{
    assert(buf->readableSize() == 0);
    // FIXME: can we move serialization & checksum to other thread?
    buf->append(tag_);
    int byte_szie = serializeToBuffer(message, buf);
    int32_t checksum_val = checksum(buf->peek(), static_cast<int>(buf->readableSize()));
    buf->appendInt32(checksum_val);
    assert(buf->readableSize() == tag_.size() + byte_szie + kChecksumLen);
    (void)byte_szie;
    int len = sockets::hostToNetwork32(buf->readableSize());
    buf->prepend(&len, sizeof(len));
}

int ProtobufCodecLite::serializeToBuffer(const ::google::protobuf::Message& message, Buffer* buf)
{
    // TODO: use BufferOutputStrean
    // BufferOutputStream os(buf);
    // message.SerializeToZeroCopyStream(&os);
    // return static_cast<int>(os.ByteCount);

    // code copied from MessageLite::SerializeToArray() and MessageLite::SerializePartialToArray().
    GOOGLE_CHECK(message.IsInitialized()) << InitializationErrorMessage("serialize", message);
    int byte_size = message.ByteSize();
    buf->ensureWriteableSize(byte_size + kChecksumLen);
    uint8_t* start = reinterpret_cast<uint8_t*>(buf->beginWrite());
    uint8_t* end = message.SerializeWithCachedSizesToArray(start);
    if (end - start != byte_size)
    {
        ByteSizeConsistencyError(byte_size, message.ByteSize(), static_cast<int>(end - start));
    }
    buf->haveWritten(byte_size);
    return byte_size;
}

int32_t ProtobufCodecLite::checksum(const void* buf, int len)
{
    return static_cast<int32_t>(::adler32(1, static_cast<const Bytef*>(buf), static_cast<uInt>(len)));
}

void ProtobufCodecLite::onMessasge(const TcpConnectionPtr& connection,
                                   Buffer* buf,
                                   Timestamp receive_time)
{
    while (buf->readableSize() >= static_cast<uint32_t>(kMinMessageLen + kHeaderLen))
    {
        const int32_t len = buf->peekInt32();
        if (len > kMaxMessageLen || len < kMinMessageLen)
        {
            error_callback_(connection, buf, receive_time, kInvalidLength);
            break;
        }
        else if (buf->readableSize() >= implicit_cast<size_t>(kHeaderLen + len))
        {
            if (raw_callback_ &&
                raw_callback_(connection, StringPiece(buf->peek(), kHeaderLen + len), receive_time))
            {
                buf->reset(kHeaderLen + len);
                continue;
            }
            MessagePtr message(proto_type_->New());
            // FIXME: can we move deserialization & callback to other thread?
            ErrorCode error_code = parse(buf->peek() + kHeaderLen, len, message.get());
            if (error_code == kNoError)
            {
                // FIXME: tre {} catch(...) {}
                message_callback_(connection, message, receive_time);
                buf->reset(kHeaderLen + len);
            }
            else
            {
                error_callback_(connection, buf, receive_time, error_code);
                break;
            }
        }
        else
        {
            break;
        }
    }
}

ProtobufCodecLite::ErrorCode ProtobufCodecLite::parse(const char* buf,
                                                      int len,
                                                      ::google::protobuf::Message* message)
{
    ErrorCode error = kNoError;
    if (validateChecksum(buf, len))
    {
        if (memcmp(buf, tag_.data(), tag_.size()))
        {
            // parse from buffer
            const char* data = buf + tag_.size();
            int32_t data_len = len - kChecksumLen - static_cast<int>(tag_.size());
            if (parseFromBuffer(StringPiece(data, data_len), message))
            {
                error = kNoError;
            }
            else
            {
                error = kParseError;
            }
        }
        else
        {
            error = kUnknownMessageType;
        }
    }
    else
    {
        error = kChecksumError;
    }
    return error;
}

bool ProtobufCodecLite::validateChecksum(const char* buf, int len)
{
    // check sum
    int32_t expected_checksum = asInt32(buf + len - kChecksumLen);
    int32_t checksum_val = checksum(buf, len - kChecksumLen);
    return checksum_val == expected_checksum;
}

int32_t ProtobufCodecLite::asInt32(const char* buf)
{
    int32_t be32 = 0;
    memcpy(&be32, buf, sizeof(be32));
    return blink::sockets::networkToHost32(be32);
}

bool ProtobufCodecLite::parseFromBuffer(StringPiece buf, ::google::protobuf::Message* message)
{
    return message->ParseFromArray(buf.data(), buf.size());
}

const string& ProtobufCodecLite::errorCodeToString(ErrorCode error_code)
{
    switch (error_code)
    {
        case kNoError:
            return kNoErrorStr;
        case kInvalidLength:
            return kInvalidLengthStr;
        case kChecksumError:
            return kChecksumErrorStr;
        case kInvalidNameLen:
            return kInvalidNameLenStr;
        case kUnknownMessageType:
            return kUnknownMessageTypeStr;
        case kParseError:
            return kParseErrorStr;
        default:
            return kUnknownErrorStr;
    }
}

void ProtobufCodecLite::defaultErrorCallback(const TcpConnectionPtr& connection,
                                             Buffer* buf,
                                             Timestamp,
                                             ErrorCode error_code)
{
    LOG_ERROR << "ProtobufCodecLite::defaultErrorCallback - " << errorCodeToString(error_code);
    if (connection && connection->connected())
    {
        connection->shutdown();
    }
}

}  // namespace blink
