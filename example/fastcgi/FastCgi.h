#ifndef __EXAMPLE_FASTCGI_H__
#define __EXAMPLE_FASTCGI_H__

#include "TcpConnection.h"
#include "Nocopyable.h"

#include <map>

// One FastCgiCodec per TcpConnection
class FastCgiCodec : blink::Nocopyable
{
public:
    typedef std::map<std::string, std::string> ParamMap;
    typedef boost::function<void (const blink::TcpConnectionPtr&,
                                  ParamMap&,
                                  blink::Buffer*)> Callback;

    explicit FastCgiCodec(const Callback& cb)
        : callback_(cb),
          got_request_(false),
          keep_connection_(false)
    {
    }

    void onMessage(const blink::TcpConnectionPtr& connection,
                   blink::Buffer* buf,
                   blink::Timestamp receive_time)
    {
        parseRequest(buf);
        if (got_request_)
        {
            callback_(connection, params_, &stdin_);
            stdin_.resetAll();
            params_stream_.resetAll();
            params_.clear();
            got_request_ = false;
            if (!keep_connection_)
            {
                connection->shutdown();
            }
        }
    }

    static void respond(blink::Buffer* response);

private:
    struct RecordHeader;

    bool onParams(const char* content, uint16_t length);
    void onStdin(const char* content, uint16_t length);
    bool onBeginRequest(const RecordHeader& header, const blink::Buffer* buf);
    bool parseRequest(blink::Buffer* buf);
    bool parseAllParams();
    uint32_t readLen();

    static void endStdout(blink::Buffer* buf);
    static void endRequest(blink::Buffer* buf);

    Callback       callback_;
    bool           got_request_;
    bool           keep_connection_;
    blink::Buffer  stdin_;
    blink::Buffer  params_stream_;
    ParamMap       params_;

    static const unsigned kRecordHeader;
};

#endif
