#ifndef __BLINK_HTTP_HTTPRESPONSE_H__
#define __BLINK_HTTP_HTTPRESPONSE_H__

#include <blink/Copyable.h>
#include <blink/Types.h>

#include <map>

namespace blink
{

class Buffer;

class HttpResponse : Copyable
{
public:
    enum HttpStatusCode
    {
        kUnknown,
        kOk = 200,
        kMovePermanently = 301,
        kBadReqeust = 400,
        kNotFound = 404,
    };

    explicit HttpResponse(bool close)
        :status_code_(kUnknown), close_connection_(close)
    {
    }

    void appendToBuffer(Buffer* output) const;

    void setStatusCode(HttpStatusCode code)
    {
        status_code_ = code;
    }

    void setStatusMessage(const string& message)
    {
        status_message_ = message;
    }

    void setCloseConnection(bool on)
    {
        close_connection_ = on;
    }

    bool closeConnection() const
    {
        return close_connection_;
    }

    void setContextType(const string& context_type)
    {
        addHeader("Context-Type", context_type);
    }

    void addHeader(const string& key, const string& value)
    {
        headers_[key] = value;
    }

    void setBody(const string& body)
    {
        body_ = body;
    }

private:
    std::map<string, string>  headers_;
    HttpStatusCode            status_code_;        // FIXME: add http version
    string                    status_message_;
    bool                      close_connection_;
    string                    body_;
};

}  // namespace blink

#endif
