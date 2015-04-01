#ifndef __BLINK_LOGSTREAM_H__
#define __BLINK_LOGSTREAM_H__

#include <blink/Nocopyable.h>
#include <blink/Copyable.h>
#include <blink/Types.h>
#include <blink/StringPiece.h>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_arithmetic.hpp>

#include <algorithm>

#ifndef BLINK_STD_STRING
#include <string>
#endif

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

namespace blink
{

template<int SIZE>
class FixedBuffer : Nocopyable
{
public:
    FixedBuffer()
        : current_(buf_)
    {
        setCookie(cookieStart);
    }

    ~FixedBuffer()
    {
        setCookie(cookieEnd);
    }

    void setCookie(void (*cookie)())
    {
        cookie_ = cookie;
    }

    char* current()
    {
        return current_;
    }

    size_t usedSize() const
    {
        return implicit_cast<size_t>(current_ - buf_);
    }

    size_t availableSize() const
    {
        return implicit_cast<size_t>(end() - current_);
    }

    void append(const char* buf, size_t len)
    {
        if (availableSize() > len)
        {
            memcpy(current_, buf, len);
            current_ += len;
        }
    }

    void add(size_t len)
    {
        current_ += len;
    }

    void reset()
    {
        current_ = buf_;
    }

    void memset()
    {
        ::memset(buf_, 0, sizeof(buf_));
    }

    const char* data() const
    {
        return buf_;
    }

    string toString() const
    {
        return string(buf_, current_ - buf_);
    }

    StringPiece toStringPiece() const
    {
        return StringPiece(buf_, current_ - buf_)
    }

    const char* debugString() const
    {
        *current_ = '\0';
        return buf_;
    }

private:
    const char* end() const
    {
        return buf_ + sizeof(buf_);
    }

    static void cookieStart()
    {
    }

    static void cookieEnd()
    {
    }

    char   buf_[SIZE];
    char*  current_;
    void   (*cookie_)();
};

class LogStream
{
public:
    static const int kSmallBuffer = 4000;
    static const int kLargeBuffer = 4000 * 1000;

    typedef FixedBuffer<kSmallBuffer> Buffer;

    void append(const char* data, size_t len);
    void reset();

    LogStream& operator<<(short x);
    LogStream& operator<<(unsigned short x);
    LogStream& operator<<(int x);
    LogStream& operator<<(unsigned int x);
    LogStream& operator<<(long x);
    LogStream& operator<<(unsigned long x);
    LogStream& operator<<(long long x);
    LogStream& operator<<(unsigned long long x);

    LogStream& operator<<(const void* x);

    LogStream& operator<<(double x);

    LogStream& operator<<(float x)
    {
        return operator<<(static_cast<double>(x));
    }

    LogStream& operator<<(bool x)
    {
        buffer_.append(x ? "1" : "0", 1);
        return *this;
    }

    LogStream& operator<<(char x)
    {
        buffer_.append(&x, 1);
        return *this;
    }

    LogStream& operator<<(const char* str)
    {
        if (str)
        {
            buffer_.append(str, strlen(str));
        }
        else
        {
            buffer_.append("(null)", 6);
        }
        return *this;
    }

    LogStream& operator<<(const unsigned char* str)
    {
        return operator<<(reinterpret_cast<const char*>(str));
    }

    LogStream& operator<<(const string& s)
    {
        buffer_.append(s.c_str(), s.size());
        return *this;
    }

#ifndef BLINK_STD_STRING
    LogStream& operator<<(const std::string& s)
    {
        buffer_.append(s.c_str(), s.size());
        return *this;
    }
#endif

    LogStream& operator<<(const StringPiece& s)
    {
        buffer_.append(s.data(), s.size());
        return *this;
    }

    LogStream& operator<<(const Buffer& v)
    {
        *this << v.toStringPiece();
        return *this;
    }

    const Buffer& buffer() const
    {
        return buffer_;
    }

private:
    void staticCheck();

    template<typename T>
    void formatInteger(T x);

    Buffer buffer_;
    static const int kMaxNumericSize = 32;
};

class Format : Copyable
{
public:
    template<typename T>
    Format(const char* fmt, T val)
    {
        BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value == true);  //T must be integer type or floating-point type
        length_ = snprintf(buf_, sizeof(buf_), fmt, val);
        assert(length_ < sizeof(buf_));
    }

    const char* data() const
    {
        return buf_;
    }

    size_t length() const
    {
        return length_;
    }

private:
    char    buf_[32];
    size_t  length_;
};

inline LogStream& operator<<(LogStream& lhs, const Format& fmt)
{
    lhs.append(fmt.data(), fmt.length());
    return lhs;
}

}  // namespace blink

#endif

