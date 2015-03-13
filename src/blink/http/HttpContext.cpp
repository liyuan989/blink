#include <blink/http/HttpContext.h>
#include <blink/Buffer.h>

namespace blink
{

bool HttpContext::parseRequest(Buffer* buf, Timestamp receive_time)
{
    bool ok = true;
    bool has_more = true;
    while (has_more)
    {
        if (expectRequestLine())
        {
            const char* crlf = buf->findCRLF();
            if (crlf)
            {
                ok = processRequestLine(buf->peek(), crlf);
                if (ok)
                {
                    request_.setReceiveTime(receive_time);
                    buf->resetUntil(crlf + 2);
                    receiveRequestLine();
                }
                else
                {
                    has_more = false;
                }
            }
            else
            {
                has_more = false;
            }
        }
        else if (expectHeaders())
        {
            const char* crlf = buf->findCRLF();
            if (crlf)
            {
                const char* colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf)
                {
                    request_.addHeader(buf->peek(), colon, crlf);
                }
                else
                {
                    // empty line, end of header
                    receiveHeaders();
                    has_more = !gotAll();
                }
                buf->resetUntil(crlf + 2);
            }
            else
            {
                has_more = false;
            }
        }
        else if (expectBody())
        {
            // FIXME:
        }
    }
    return ok;
}

bool HttpContext::processRequestLine(const char* begin, const char* end)
{
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    if (space != end && request_.setMethod(start, space))
    {
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end)
        {
            const char* question = std::find(start, space, '?');
            if (question != space)
            {
                request_.setPath(start, question);
                request_.setQuery(question, space);
            }
            else
            {
                request_.setPath(start, space);
            }
            start = space + 1;
            succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
            if (succeed)
            {
                if (*(end - 1) == '1')
                {
                    request_.setVersion(HttpRequest::kHttp11);
                }
                else if (*(end - 1) == '0')
                {
                    request_.setVersion(HttpRequest::kHttp10);
                }
                else
                {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

}  // namespace blink
