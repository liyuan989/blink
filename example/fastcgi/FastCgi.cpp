#include "FastCgi.h"
#include "Endian.h"
#include "Log.h"

using namespace blink;

struct FastCgiCodec::RecordHeader
{
    uint8_t   version;
    uint8_t   type;
    uint16_t  id;
    uint16_t  length;
    uint8_t   padding;
    uint8_t   unused;
};

enum FastCgiType
{
    kFcgiInvalid = 0,
    kFcgiBeginRequest = 1,
    kFcgiAbortRequest = 2,
    kFcgiEndRequest = 3,
    kFcgiParams = 4,
    kFcgiStdin = 5,
    kFcgiStdout = 6,
    kFcgiStderr = 7,
    kFcgiData = 8,
    kFcgiGetValues = 9,
    kFcgiGetValuesResult = 10,
    kFcgiUnknownType = 11,
};

enum FastCgiRole
{
    //kFcgiInvalid = 0,
    kFcgiResponder = 1,
    kFcgiAuthorizer = 2,
};

enum FastCgiConstant
{
    kFcgikeepConnection = 1,
};

uint16_t readInt16(const void* p)
{
    uint16_t be16 = 0;
    memcpy(&be16, p, sizeof(be16));
    return sockets::networkToHost16(be16);
}

const unsigned FastCgiCodec::kRecordHeader = static_cast<unsigned>(sizeof(FastCgiCodec::RecordHeader));

bool FastCgiCodec::parseRequest(Buffer* buf)
{
    while (buf->readableSize() >= kRecordHeader)
    {
        RecordHeader header;
        memcpy(&header, buf->peek(), kRecordHeader);
        header.id = sockets::networkToHost16(header.id);
        header.length = sockets::networkToHost16(header.length);
        size_t total = kRecordHeader + header.length + header.padding;
        if (buf->readableSize() >= total)
        {
            switch (header.type)
            {
                case kFcgiBeginRequest:
                    onBeginRequest(header, buf);
                    break;
                case kFcgiParams:
                    onParams(buf->peek() + kRecordHeader, header.length);
                    break;
                case kFcgiStdin:
                    onStdin(buf->peek() + kRecordHeader, header.length);
                    break;
                case kFcgiData:
                    break;
                case kFcgiGetValues:
                    break;
                default:
                    break;
            }
            buf->reset(total);
        }
        else
        {
            break;
        }
    }
    return true;
}

bool FastCgiCodec::onBeginRequest(const RecordHeader& header, const Buffer* buf)
{
    assert(buf->readableSize() >= header.length);
    assert(header.type == kFcgiBeginRequest);
    if (header.length > kRecordHeader)
    {
        uint16_t role = readInt16(buf->peek() + kRecordHeader);
        uint8_t flags = buf->peek()[kRecordHeader + sizeof(int16_t)];
        if (role == kFcgiResponder)
        {
            keep_connection_ = flags == kFcgikeepConnection;
            return true;
        }
    }
    return false;
}


bool FastCgiCodec::onParams(const char* content, uint16_t length)
{
    if (length > 0)
    {
        params_stream_.append(content, length);
    }
    else if (!parseAllParams())
    {
        LOG_ERROR << "parseAllParams() falied";
        return false;
    }
    return true;
}

bool FastCgiCodec::parseAllParams()
{
    while (params_stream_.readableSize() > 0)
    {
        uint32_t name_len = readLen();
        if (name_len == static_cast<uint32_t>(-1))
        {
            return false;
        }
        uint32_t value_len = readLen();
        if (value_len == static_cast<uint32_t>(-1))
        {
            return false;
        }
        if (params_stream_.readableSize() >= name_len + value_len)
        {
            string name = params_stream_.resetToString(name_len);
            params_[name] = params_stream_.resetToString(value_len);
        }
        else
        {
            return false;
        }
    }
    return true;
}

uint32_t FastCgiCodec::readLen()
{
    if (params_stream_.readableSize() >= 1)
    {
        uint8_t byte = params_stream_.peekInt8();
        if (byte & 0x80)
        {
            if (params_stream_.readableSize() >= sizeof(uint32_t))
            {
                return params_stream_.readInt32() & 0x7fffffff;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return params_stream_.readInt8();
        }
    }
    else
    {
        return -1;
    }
}

void FastCgiCodec::onStdin(const char* content, uint16_t length)
{
    if (length > 0)
    {
        stdin_.append(content, length);
    }
    else
    {
        got_request_ = true;
    }
}

void FastCgiCodec::respond(Buffer* response)
{
    if (response->readableSize() < 65536
        && response->prependableSize() >= kRecordHeader)
    {
        RecordHeader header =
        {
            1,
            kFcgiStdout,
            sockets::hostToNetwork16(1),
            sockets::hostToNetwork16(static_cast<uint16_t>(response->readableSize())),
            static_cast<uint8_t>(-response->readableSize() & 7),    // NOTE: equal to (8 - x % 8)
            0,
        };
        response->prepend(&header, kRecordHeader);
        response->append("\0\0\0\0\0\0\0\0", header.padding);
    }
    else
    {
        // FIXME:
    }
    endStdout(response);
    endRequest(response);
}

void FastCgiCodec::endStdout(Buffer* buf)
{
    RecordHeader header =
    {
        1,
        kFcgiStdout,
        sockets::hostToNetwork16(1),
        0,
        0,
        0,
    };
    buf->append(&header, kRecordHeader);
}

void FastCgiCodec::endRequest(Buffer* buf)
{
    RecordHeader header =
    {
        1,
        kFcgiEndRequest,
        sockets::hostToNetwork16(1),
        sockets::hostToNetwork16(kRecordHeader),
        0,
        0,
    };
    buf->append(&header, kRecordHeader);
    buf->appendInt32(0);
    buf->appendInt32(0);
}
