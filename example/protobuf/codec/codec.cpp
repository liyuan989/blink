#include <example/protobuf/codec/codec.h>

#include <blink/protorpc/google-inl.h>
#include <blink/Endian.h>
#include <blink/Log.h>

#include <google/protobuf/descriptor.h>

#include <zlib.h>

using namespace blink;

namespace
{

const string kNoErrorStr = "NoError";
const string kInvalidLengthStr = "InvalidLength";
const string kChecksumErrorStr = "ChecksumError";
const string kInvalidNameLenStr = "InvalidNameLen";
const string kUnknownMessageTypeStr = "UnKnownMessageType";
const string kParseErrorStr = "ParseError";
const string kUnknownErrorStr = "UnknownError";

}  // anonymous namespace

const int ProtobufCodec::kHeaderLen;
const int ProtobufCodec::kMinMessageLen;
const int ProtobufCodec::kMaxMessageLen;

void ProtobufCodec::fillEmptyBuffer(Buffer* buf, const google::protobuf::Message& message)
{
    assert(buf->readableSize() == 0);
    const std::string& type_name = message.GetTypeName();
    int32_t name_len = static_cast<int32_t>(type_name.size());
    buf->appendInt32(name_len);
    buf->append(type_name.c_str(), name_len);
    // code copied from MessageLite::SerializeToArray() and MessageLite::SerializePartialToArray()
    GOOGLE_DCHECK(message.IsInitialized()) << InitializationErrorMessage("serialize", message);

    int byte_size= message.ByteSize();
    buf->ensureWriteableSize(byte_size);

    uint8_t* start = reinterpret_cast<uint8_t*>(buf->beginWrite());
    uint8_t* end = message.SerializeWithCachedSizesToArray(start);
    if (end - start != byte_size)
    {
        ByteSizeConsistencyError(byte_size, message.ByteSize(), static_cast<int>(end - start));
    }
    buf->haveWritten(byte_size);

    int32_t checksum = static_cast<int32_t>(
        adler32(1,
                reinterpret_cast<const Bytef*>(buf->peek()),
                static_cast<uInt>(buf->readableSize())));
    buf->appendInt32(checksum);
    assert(buf->readableSize() == sizeof(name_len) + name_len + byte_size + sizeof(checksum));
    buf->prependInt32(static_cast<int32_t>(buf->readableSize()));
}

void ProtobufCodec::onMessage(const TcpConnectionPtr& connection,
                              Buffer* buf,
                              Timestamp receive_time)
{
    while (buf->readableSize() >= kMinMessageLen + kHeaderLen)
    {
        const int32_t len = buf->peekInt32();
        if (len > kMaxMessageLen || len < kMinMessageLen)
        {
            error_callback_(connection, buf, receive_time, kInvalidLength);
            break;
        }
        else if (buf->readableSize() >= static_cast<size_t>(len + kHeaderLen))
        {
            ErrorCode error_code = kNoError;
            MessagePtr message = parse(buf->peek() + kHeaderLen, len, &error_code);
            if (error_code == kNoError && message)
            {
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

int32_t asInt32(const char* buf)
{
    int32_t be32 = 0;
    memcpy(&be32, buf, sizeof(be32));
    return sockets::networkToHost32(be32);
}

MessagePtr ProtobufCodec::parse(const char* buf, int len, ErrorCode* error_code)
{
    MessagePtr message;
    int32_t expected_checksum = asInt32(buf + len - kHeaderLen);
    int32_t checksum = static_cast<int32_t>(
        adler32(1,
                reinterpret_cast<const Bytef*>(buf),
                static_cast<uInt>(len - kHeaderLen)));
    if (checksum == expected_checksum)
    {
        int32_t name_len = asInt32(buf);
        if (name_len > 2 && name_len <= len - 2 * kHeaderLen)
        {
            std::string type_name(buf + kHeaderLen, buf + kHeaderLen + name_len);
            message.reset(createMessage(type_name));
            if (message)
            {
                const char* data = buf + kHeaderLen + name_len;
                int32_t data_len = len - 2 * kHeaderLen - name_len;
                if (message->ParseFromArray(data, data_len))
                {
                    *error_code = kNoError;
                }
                else
                {
                    *error_code = kParseError;
                }
            }
            else
            {
                *error_code = kUnknownMessageType;
            }
        }
        else
        {
            *error_code = kInvalidNameLen;
        }
    }
    else
    {
        *error_code = kChecksumError;
    }
    return message;
}

google::protobuf::Message* ProtobufCodec::createMessage(const std::string& type_name)
{
    google::protobuf::Message* message = NULL;
    const google::protobuf::Descriptor* descriptor =
        google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
    if (descriptor)
    {
        const google::protobuf::Message* prototype =
            google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
        if (prototype)
        {
            message = prototype->New();
        }
    }
    return message;
}

const string& ProtobufCodec::errorCodeToString(ErrorCode error_code)
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

void ProtobufCodec::defaultErrorCallback(const TcpConnectionPtr& connection,
                                         Buffer* buf,
                                         Timestamp receive_time,
                                         ErrorCode error_code)
{
    LOG_ERROR << "ProtobufCodec::defaultErrorCallback - " << errorCodeToString(error_code);
    if (connection->connected())
    {
        connection->shutdown();
    }
}
