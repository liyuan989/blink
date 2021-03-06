#include <blink/LogStream.h>

#include <limits>
#include <stdint.h>
#include <stdio.h>

#pragma GCC diagnostic ignored "-Wtype-limits"

namespace blink
{

namespace detail
{

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

size_t convertHex(char* buf, uintptr_t val)  // ptr address to string convertions.
{
    const char digits_hex[] = "0123456789ABCDEF";
    uintptr_t i = val;
    char* p = buf;
    do
    {
        int lsd = static_cast<int>(i % 16);
        i /= 16;
        *p = digits_hex[lsd];
        ++p;
    } while (i != 0);

    *p = '\0';
    std::reverse(buf, p);
    return p - buf;
}

}  // namespace detail

const int LogStream::kSmallBuffer;
const int LogStream::kLargeBuffer;
const int LogStream::kMaxNumericSize;

void LogStream::append(const char* data, size_t len)
{
    buffer_.append(data, len);
}

void LogStream::reset()
{
    buffer_.reset();
}

LogStream& LogStream::operator<<(short x)
{
    return operator<<(static_cast<int>(x));
}

LogStream& LogStream::operator<<(unsigned short x)
{
    return operator<<(static_cast<unsigned int>(x));
}

LogStream& LogStream::operator<<(int x)
{
    formatInteger(x);
    return *this;
}

LogStream& LogStream::operator<<(unsigned int x)
{
    formatInteger(x);
    return *this;
}

LogStream& LogStream::operator<<(long x)
{
    formatInteger(x);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long x)
{
    formatInteger(x);
    return *this;
}

LogStream& LogStream::operator<<(long long x)
{
    formatInteger(x);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long x)
{
    formatInteger(x);
    return *this;
}

LogStream& LogStream::operator<<(const void* x)
{
    uintptr_t p = reinterpret_cast<uintptr_t>(x);
    if (buffer_.availableSize() > static_cast<size_t>(kMaxNumericSize))
    {
        char* cur = buffer_.current();
        cur[0] = '0';
        cur[1] = 'x';
        size_t len = detail::convertHex(cur + 2, p);
        buffer_.add(len + 2);
    }
    return *this;
}

LogStream& LogStream::operator<<(double x)
{
    if (buffer_.availableSize() >= static_cast<size_t>(kMaxNumericSize))
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", x);
        buffer_.add(len);
    }
    return *this;
}

void LogStream::staticCheck()
{
    BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10);
    BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10);
    BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10);
    BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10);
}

template<typename T>
void LogStream::formatInteger(T x)
{
    if (buffer_.availableSize() >= static_cast<size_t>(kMaxNumericSize))
    {
        size_t len = detail::convert(buffer_.current(), x);
        buffer_.add(len);
    }
}

}  // namespace blink
