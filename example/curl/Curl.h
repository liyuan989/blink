#ifndef __EXAMPLE_CURL_CURL_H__
#define __EXAMPLE_CURL_CURL_H__

#include <blink/StringPiece.h>
#include <blink/Nocopyable.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>

extern "C"
{

typedef void CURLM;
typedef void CURL;

}

namespace blink
{

class Channel;
class EventLoop;

}  // namespace blink

class Curl;

class Request : blink::Nocopyable,
                public boost::enable_shared_from_this<Request>
{
public:
    typedef boost::function<void (const char*, int)> DataCallback;
    typedef boost::function<void (Request*, int)> DoneCallback;

    Request(Curl* owner, const char* url);
    ~Request();

    void setDataCallback(const DataCallback& cb)
    {
        data_callback_ = cb;
    }

    void setDoneCallback(const DoneCallback& cb)
    {
        done_callback_ = cb;
    }

    void setHeaderCallback(const DataCallback& cb)
    {
        header_callback_ = cb;
    }

    template<typename OPT>
    int setopt(OPT opt, long p)
    {
        return curl_easy_setopt(curl_, opt, p);
    }

    template<typename OPT>
    int setopt(OPT opt, const char* p)
    {
        return curl_easy_setopt(curl_, opt, p);
    }

    template<typename OPT>
    int setopt(OPT opt, void* p)
    {
        return curl_easy_setopt(curl_, opt, p);
    }

    template<typename OPT>
    int setopt(OPT opt, size_t (*p)(char*, size_t, size_t, void*))
    {
        return curl_easy_setopt(curl_, opt, p);
    }

    //void allowRediect(int redirects);
    void headerOnly();
    void setRange(const blink::StringArg range);
    const char* getEffectiveUrl();
    const char* getRedirectUrl();
    int getResponseCode();

    // internal
    CURL* getCurl()
    {
        return curl_;
    }

    blink::Channel* getChannel()
    {
        return boost::get_pointer(channel_);
    }

    blink::Channel* setChannel(int fd);
    void removeChannel();
    void done(int code);

private:
    void dataCallback(const char* buffer, int len);
    void headerCallback(const char* buffer, int len);
    void doneCallback();

    static size_t writeData(char* buffer, size_t size, size_t nmemb, void* userp);
    static size_t headerData(char* buffer, size_t size, size_t nmemb, void* userp);

    Curl*                              owner_;
    CURL*                              curl_;
    boost::shared_ptr<blink::Channel>  channel_;
    DataCallback                       data_callback_;
    DataCallback                       header_callback_;
    DoneCallback                       done_callback_;
};

typedef boost::shared_ptr<Request> RequestPtr;

class Curl : blink::Nocopyable
{
public:
    enum Option
    {
        kCurlNoSsl = 0,
        kCurlSsl,
    };

    explicit Curl(blink::EventLoop* loop);
    ~Curl();

    RequestPtr getUrl(blink::StringArg url);

    static void initialize(Option opt = kCurlNoSsl);

    // internal
    CURLM* getCurlm()
    {
        return curlm_;
    }

    blink::EventLoop* getLoop()
    {
        return loop_;
    }

private:
    void onTimer();
    void onRead(int fd);
    void onWrite(int fd);
    void checkFinish();

    static int socketCallback(CURL* c, int fd, int what, void* userp, void* socketp);
    static int timerCallback(CURLM* curlm, long ms, void* userp);

    blink::EventLoop*  loop_;
    CURLM*             curlm_;
    int                running_handles_;
    int                prev_running_handles_;
};

#endif
