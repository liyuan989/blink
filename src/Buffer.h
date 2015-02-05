#ifndef __BLINK_BUFFER_H__
#define __BLINK_BUFFER_H__

#include "Copyable.h"
#include "Endian.h"
#include "Types.h"
#include "StringPiece.h"

#include <algorithm>
#include <vector>
#include <stdint.h>
#include <assert.h>
#include <string.h>

namespace blink
{

class Buffer : Copyable
{
public:
    static const int kPrependSize = 8;
    static const int kBufferSize = 1024;

    explicit Buffer(size_t size = kBufferSize)
        : buffer_(kPrependSize + size), read_index_(kPrependSize), write_index_(kPrependSize)
    {
        assert(readableSize() == 0);
        assert(writeableSize() == size);
        assert(prependableSize() == kPrependSize);
    }

    ssize_t readData(int fd, int* err);

    size_t bufferCapacity() const
    {
        return buffer_.capacity();
    }

    size_t readableSize() const
    {
        return write_index_ - read_index_;
    }

    size_t writeableSize() const
    {
        return buffer_.size() - write_index_;
    }

    size_t prependableSize() const
    {
        return read_index_;
    }

    void shrink(size_t reserve)
    {
        Buffer temp;
        temp.ensureWriteableSize(readableSize() + reserve);
        temp.append(toStringPiece());
        swap(temp);
    }

    void swap(Buffer& rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(read_index_, rhs.read_index_);
        std::swap(write_index_, rhs.write_index_);
    }

    char* beginWrite()
    {
        return begin() + write_index_;
    }

    const char* beginWrite() const
    {
        return begin() + write_index_;
    }

    const char* peek() const
    {
        return begin() + read_index_;
    }

    int8_t peekInt8() const
    {
        assert(readableSize() >= sizeof(int8_t));
        int8_t x = *peek();
        return x;
    }

    int16_t peekInt16() const
    {
        assert(readableSize() >= sizeof(int16_t));
        int16_t x = 0;
        memcpy(&x, peek(), sizeof(x));
        return x;
    }

    int32_t peekInt32() const
    {
        assert(readableSize() >= sizeof(int32_t));
        int32_t x = 0;
        memcpy(&x, peek(), sizeof(x));
        return x;
    }

    int64_t peekInt64() const
    {
        assert(readableSize() >= sizeof(int64_t));
        int64_t x = 0;
        memcpy(&x, peek(), sizeof(x));
        return x;
    }

    const char* findCRLF() const
    {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findCRLF(const char* start) const
    {
        if (start >= peek() && start <= beginWrite())
        {
            const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
            return crlf == beginWrite() ? NULL : crlf;
        }
        else
        {
            return NULL;
        }
    }

    const char* findEndOfLine() const
    {
        const char* eol = std::find(peek(), beginWrite(), '\n');
        return eol == beginWrite() ? NULL : eol;
    }

    const char* findEndOfLine(const char* start) const
    {
        if (start >= peek() && start <= beginWrite())
        {
            const char* eol = std::find(start, beginWrite(), '\n');
            return eol == beginWrite() ? NULL : eol;
        }
        else
        {
            return NULL;
        }
    }

    void reset(size_t len)
    {
        assert(len <= readableSize());
        if (len < readableSize())
        {
            read_index_ += len;
        }
        else // len == readableSize()
        {
            resetAll();
        }
    }

    void resetAll()
    {
        read_index_ = kPrependSize;
        write_index_ = kPrependSize;
    }

    void resetUntil(const char* end)
    {
        assert(end <= beginWrite());
        assert(end >= peek());
        reset(end - peek());
    }

    void resetInt8()
    {
        reset(sizeof(int8_t));
    }

    void resetInt16()
    {
        reset(sizeof(int16_t));
    }

    void resetInt32()
    {
        reset(sizeof(int32_t));
    }

    void resetInt64()
    {
        reset(sizeof(int64_t));
    }

    string resetToString(size_t len)
    {
        assert(len <= readableSize());
        string result(peek(), len);
        reset(len);
        return result;
    }

    string resetAllToString()
    {
        return resetToString(readableSize());
    }

    string toString() const
    {
        return string(peek(), readableSize());
    }

    StringPiece toStringPiece()
    {
        return StringPiece(peek(), static_cast<int>(readableSize()));
    }

    void ensureWriteableSize(size_t len)
    {
        if (len > writeableSize())
        {
            makeSpace(len);
        }
        assert(writeableSize() >= len);
    }

    void append(const char* data, size_t len)
    {
        ensureWriteableSize(len);
        std::copy(data, data + len, beginWrite());
        haveWritten(len);
    }

    void append(const void* data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }

    void append(const StringPiece& str)
    {
        append(str.data(), str.size());
    }

    void appendInt8(int8_t x)
    {
        append(&x, sizeof(x));
    }

    void appendInt16(int16_t x)
    {
        int16_t be16 = sockets::hostToNetwork16(x);
        append(&be16, sizeof(be16));
    }

    void appendInt32(int32_t x)
    {
        int32_t be32 = sockets::hostToNetwork32(x);
        append(&be32, sizeof(be32));
    }

    void appendInt64(int64_t x)
    {
        int64_t be64 = sockets::hostToNetwork64(x);
        append(&be64, sizeof(be64));
    }

    int8_t readInt8()
    {
        int8_t x = peekInt8();
        resetInt8();
        return x;
    }

    int16_t readInt16()
    {
        int16_t x = peekInt16();
        resetInt16();
        return x;
    }

    int32_t readInt32()
    {
        int32_t x = peekInt32();
        resetInt32();
        return x;
    }

    int64_t readInt64()
    {
        int64_t x = peekInt64();
        resetInt64();
        return x;
    }

    void prepend(const void* data, size_t len)
    {
        assert(len <= prependableSize());
        read_index_ -= len;
        const char* dest = static_cast<const char*>(data);
        std::copy(dest, dest + len, begin() + read_index_);
    }

    void prependInt8(int8_t x)
    {
        prepend(&x, sizeof(int8_t));
    }

    void prependInt16(int16_t x)
    {
        int16_t be16 = sockets::hostToNetwork16(x);
        prepend(&be16, sizeof(be16));
    }

    void prependInt32(int32_t x)
    {
        int32_t be32 = sockets::hostToNetwork32(x);
        prepend(&be32, sizeof(be32));
    }

    void prependInt64(int64_t x)
    {
        int64_t be64 = sockets::hostToNetwork64(x);
        prepend(&be64, sizeof(be64));
    }

private:
    char* begin()
    {
        return &*buffer_.begin();
    }

    const char* begin() const
    {
        return &*buffer_.begin();
    }

    void haveWritten(size_t len)
    {
        assert(len <= writeableSize());
        write_index_ += len;
    }

    void unwrite(size_t len)
    {
        assert(len <= readableSize());
        write_index_ -= len;
    }

    void makeSpace(size_t len)
    {
        if (prependableSize() + writeableSize() < len + kPrependSize)
        {
            buffer_.resize(write_index_ + len);
        }
        else
        {
            assert(kPrependSize <= read_index_);
            size_t readable_size = readableSize();
            std::copy(begin() + read_index_, begin() + write_index_, begin() + kPrependSize);
            read_index_ = kPrependSize;
            write_index_ = kPrependSize + readable_size;
        }
    }

    std::vector<char>  buffer_;
    size_t             read_index_;
    size_t             write_index_;
    static const char  kCRLF[];
};

}  // namespace blink

#endif
