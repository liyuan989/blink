#include <blink/Exception.h>

#include <execinfo.h>

#include <stdlib.h>

namespace blink
{

Exception::Exception(const string& message)
    : message_(message)
{
    fillStackTrace();
}

Exception::Exception(const char* message)
    : message_(message)
{
    fillStackTrace();
}

Exception::~Exception() throw()
{
}

const char* Exception::what() const throw()
{
    return message_.c_str();
}

const char* Exception::stackTrace() const throw()
{
    return stack_.c_str();
}

void Exception::fillStackTrace()
{
    const int len = 200;
    void* buf[len];
    int n = ::backtrace(buf, len);
    char** string_array = ::backtrace_symbols(buf, n);
    if (string_array)
    {
        for (int i = 0; i < n; ++i)
        {
            stack_.append(string_array[i]);
            stack_.push_back('\n');
        }
        free(string_array);
    }
}

}  // namespace blink
