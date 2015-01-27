#ifndef __BLINK_HTTP_HTTPREQUEST_H__
#define __BLINK_HTTP_HTTPREQUEST_H__

#include "Copyable.h"
#include "Timestamp.h"

#include <map>
#include <string>

namespace blink
{

class HttpRequest : Copyable
{
public:
    enum Method
    {
        kInvalid,
        kGet,
        kPost,
        kHead,
        kPut,
        kDelete,
    };

    enum Version
    {
        kUnknown,
        kHttp10,
        kHttp11,
    };

    HttpRequest()
        : method_(kInvalid), version_(kUnknown)
    {
    }

    bool setMethod(const char* start, const char* end);
    const char* methodString() const;
    void addHeader(const char* start, const char* colon, const char* end);
    std::string getHeader(const std::string field) const;
    void swap(HttpRequest& rhs);

    void setVersion(Version version)
    {
        version_ = version;
    }

    Version getVersion() const
    {
        return version_;
    }

    Method method() const
    {
        return method_;
    }

    void setPath(const char* start, const char* end)
    {
        path_.assign(start, end);
    }

    const std::string& path() const
    {
        return path_;
    }

    void setQuery(const char* start, const char* end)
    {
        query_.assign(start, end);
    }

    const std::string& query() const
    {
        return query_;
    }

    void setReceiveTime(Timestamp time)
    {
        receive_time_ = time;
    }

    Timestamp receiveTime() const
    {
        return receive_time_;
    }

    const std::map<std::string, std::string>& headers() const
    {
        return headers_;
    }

private:
    Method                              method_;
    Version                             version_;
    std::string                         path_;
    std::string                         query_;
    Timestamp                           receive_time_;
    std::map<std::string, std::string>  headers_;
};

}  // namespace blink

#endif
