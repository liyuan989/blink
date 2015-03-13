#ifndef __BLINK_HTTP_HTTPCONTEXT_H__
#define __BLINK_HTTP_HTTPCONTEXT_H__

#include <blink/Copyable.h>
#include <blink/http/HttpRequest.h>

namespace blink
{

class Buffer;

class HttpContext : Copyable
{
public:
    enum HttpRequestParseState
    {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };

    HttpContext()
        : state_(kExpectRequestLine)
    {
    }

    bool parseRequest(Buffer* buf, Timestamp receive_time);

    bool expectRequestLine() const
    {
        return state_ == kExpectRequestLine;
    }

    bool expectHeaders() const
    {
        return state_ == kExpectHeaders;
    }

    bool expectBody() const
    {
        return state_ == kExpectBody;
    }

    bool gotAll() const
    {
        return state_ == kGotAll;
    }

    void receiveRequestLine()
    {
        state_ = kExpectHeaders;
    }

    void receiveHeaders()
    {
        state_ = kGotAll;  // FIXME
    }

    void reset()
    {
        state_ = kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest& request() const
    {
        return request_;
    }

    HttpRequest& request()
    {
        return request_;
    }

private:
    bool processRequestLine(const char* begin, const char* end);

    HttpRequestParseState  state_;
    HttpRequest            request_;
};

}  // namespace blink

#endif
