#include "http/HttpRequest.h"

#include <assert.h>

namespace blink
{

bool HttpRequest::setMethod(const char* start, const char* end)
{
    assert(method_ == kInvalid);
    std::string str(start, end);
    if (str == "GET")
    {
        method_ = kGet;
    }
    else if (str == "POST")
    {
        method_ = kPost;
    }
    else if (str == "HEAD")
    {
        method_ = kHead;
    }
    else if (str == "PUT")
    {
        method_ = kPut;
    }
    else if (str == "DELETE")
    {
        method_ = kDelete;
    }
    else
    {
        method_ = kInvalid;
    }
    return method_ != kInvalid;
}

const char* HttpRequest::methodString() const
{
    const char* result = "UNKNOWN";
    switch (method_)
    {
        case kGet:
            result = "GET";
            break;
        case kPost:
            result = "POST";
            break;
        case kHead:
            result = "HEAD";
            break;
        case kPut:
            result = "PUT";
            break;
        case kDelete:
            result = "DELETE";
            break;
        default:
            break;
    }
    return result;
}

void HttpRequest::addHeader(const char* start, const char* colon, const char* end)
{
    std::string field(start, colon);
    ++colon;
    while (colon < end && isspace(*colon))
    {
        ++colon;
    }
    std::string value(colon, end);
    while (!value.empty() && isspace(value[value.size() - 1]))
    {
        value.resize(value.size() - 1);
    }
    headers_[field] = value;
}

std::string HttpRequest::getHeader(const std::string field) const
{
    std::string result;
    std::map<std::string, std::string>::const_iterator it = headers_.find(field);
    if (it != headers_.end())
    {
        result = it->second;
    }
    return result;
}

void HttpRequest::swap(HttpRequest& rhs)
{
    std::swap(method_, rhs.method_);
    path_.swap(rhs.path_);
    query_.swap(rhs.query_);
    receive_time_.swap(rhs.receive_time_);
    headers_.swap(rhs.headers_);
}

}  //namespace blink
