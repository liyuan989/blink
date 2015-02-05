#ifndef __BLINK_LOGSTREAM_H__
#define __BLINK_LOGSTREAM_H__

#include "Nocopyable.h"
#include "Copyable.h"
#include "Types.h"

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_arithmetic.hpp>

#include <algorithm>
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

template<typename T>
size_t convert(char* buf, T val)  // int to string conversion.  such as int a = 97 convert to char s[2] = {'9','7'}
{
    const char digits[] = "9876543210123456789";
    const char* zero = digits + 9;
    T i = val;
    char* p = buf;
    do
    {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p = zero[lsd];
        ++p;
    } while (i != 0);

    if (val < 0)
    {
        *p = '-';
        ++p;
    }
    *p = '\0';
    std::reverse(buf, p);
    return p - buf;
}

class LogStream
{
public:
    static const int kSmallBuffer = 4000;
    static const int kLargeBuffer = 4000*1000;

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

    const Buffer& buffer() const
    {
        return buffer_;
    }

private:
    void staticCheck();

    template<typename T>
    void formatInteger(T x)
    {
        if (buffer_.availableSize() >= kMaxNumericSize)
        {
            size_t len = convert(buffer_.current(), x);
            buffer_.add(len);
        }
    }

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

