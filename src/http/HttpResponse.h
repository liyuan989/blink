#ifndef __BLINK_HTTP_HTTPRESPONSE_H__
#define __BLINK_HTTP_HTTPRESPONSE_H__

#include "Copyable.h"

#include <map>
#include <string>

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

    void setStatusMessage(const std::string& message)
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

    void setContextType(const std::string& context_type)
    {
        addHeader("Context-Type", context_type);
    }

    void addHeader(const std::string& key, const std::string& value)
    {
        headers_[key] = value;
    }

    void setBody(const std::string& body)
    {
        body_ = body;
    }

private:
    std::map<std::string, std::string>  headers_;
    HttpStatusCode                      status_code_;        // FIXME: add http version
    std::string                         status_message_;
    bool                                close_connection_;
    std::string                         body_;
};

}  // namespace blink

#endif
