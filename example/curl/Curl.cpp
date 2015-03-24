#include <example/curl/Curl.h>

#include <blink/EventLoop.h>
#include <blink/Channel.h>
#include <blink/Log.h>

#include <curl/curl.h>

#include <boost/bind.hpp>

#include <assert.h>

using namespace blink;

static void dummy(const boost::shared_ptr<Channel>& channel)
{
}

Request::Request(Curl* owner, const char* url)
    : owner_(owner),
      curl_(CHECK_NOTNULL(curl_easy_init()))
{
    setopt(CURLOPT_URL, url);
    setopt(CURLOPT_WRITEFUNCTION, &Request::writeData);
    setopt(CURLOPT_WRITEDATA, this);
    setopt(CURLOPT_HEADERFUNCTION, &Request::headerData);
    setopt(CURLOPT_HEADERDATA, this);
    setopt(CURLOPT_PRIVATE, this);
    setopt(CURLOPT_USERAGENT, "curl");
    // set usragent
    LOG_DEBUG << curl_ << " " << url;
    curl_multi_add_handle(owner_->getCurlm(), curl_);
}

Request::~Request()
{
    assert(!channel_ || channel_->isNoneEvent());
    curl_multi_remove_handle(owner_->getCurlm(), curl_);
    curl_easy_cleanup(curl_);
}

// NOT implemented yet
// void Request::allowRedirect(int redirects)
// {
//     setopt(CURLOPT_FOLLOWLOCATION, 1);
//     setopt(CURLOPT_MAXREDIRS, redirects);
// }

void Request::headerOnly()
{
    setopt(CURLOPT_NOBODY, 1);
}

void Request::setRange(const StringArg range)
{
    setopt(CURLOPT_RANGE, range.c_str());
}

const char* Request::getEffectiveUrl()
{
    const char* p = NULL;
    curl_easy_getinfo(curl_, CURLINFO_EFFECTIVE_URL, &p);
    return p;
}

const char* Request::getRedirectUrl()
{
    const char* p = NULL;
    curl_easy_getinfo(curl_, CURLINFO_REDIRECT_URL, &p);
    return p;
}

int Request::getResponseCode()
{
    long code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &code);
    return static_cast<int>(code);
}

Channel* Request::setChannel(int fd)
{
    assert(channel_.get() == NULL);
    channel_.reset(new Channel(owner_->getLoop(), fd));
    channel_->tie(shared_from_this());
    return boost::get_pointer(channel_);
}

void Request::removeChannel()
{
    channel_->disableAll();
    channel_->remove();
    owner_->getLoop()->queueInLoop(boost::bind(dummy, channel_));
}

void Request::done(int code)
{
    if (done_callback_)
    {
        done_callback_(this, code);
    }
}

void Request::dataCallback(const char* buffer, int len)
{
    if (data_callback_)
    {
        data_callback_(buffer, len);
    }
}

void Request::headerCallback(const char* buffer, int len)
{
    if (header_callback_)
    {
        header_callback_(buffer, len);
    }
}

size_t Request::writeData(char* buffer, size_t size, size_t nmemb, void* userp)
{
    assert(size == 1);
    Request* request = static_cast<Request*>(userp);
    request->dataCallback(buffer, static_cast<int>(nmemb));
    return nmemb;
}

size_t Request::headerData(char* buffer, size_t size, size_t nmemb, void* userp)
{
    assert(size == 1);
    Request* request = static_cast<Request*>(userp);
    request->headerCallback(buffer, static_cast<int>(nmemb));
    return nmemb;
}

// ============================================================================

Curl::Curl(EventLoop* loop)
    : loop_(loop),
      curlm_(CHECK_NOTNULL(curl_multi_init())),
      running_handles_(0),
      prev_running_handles_(0)
{
    curl_multi_setopt(curlm_, CURLMOPT_SOCKETFUNCTION, &Curl::socketCallback);
    curl_multi_setopt(curlm_, CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(curlm_, CURLMOPT_TIMERFUNCTION, &Curl::timerCallback);
    curl_multi_setopt(curlm_, CURLMOPT_TIMERDATA, this);
}

Curl::~Curl()
{
    curl_multi_cleanup(curlm_);
}

RequestPtr Curl::getUrl(StringArg url)
{
    RequestPtr request(new Request(this, url.c_str()));
    return request;
}

void Curl::initialize(Option opt)
{
    curl_global_init(opt == kCurlNoSsl ? CURL_GLOBAL_NOTHING : CURL_GLOBAL_SSL);
}

void Curl::onTimer()
{
    CURLMcode rc = CURLM_OK;
    do
    {
        LOG_TRACE;
        rc = curl_multi_socket_action(curlm_, CURL_SOCKET_TIMEOUT, 0, &running_handles_);
        LOG_TRACE << rc << " " << running_handles_;
    } while (rc == CURLM_CALL_MULTI_PERFORM);
    checkFinish();
}

void Curl::onRead(int fd)
{
    CURLMcode rc = CURLM_OK;
    do
    {
        LOG_TRACE << fd;
        rc = curl_multi_socket_action(curlm_, fd, CURL_POLL_IN, &running_handles_);
        LOG_TRACE << rc << " " << running_handles_;
    } while (rc == CURLM_CALL_MULTI_PERFORM);
    checkFinish();
}

void Curl::onWrite(int fd)
{
    CURLMcode rc = CURLM_OK;
    do
    {
        LOG_TRACE << fd;
        rc = curl_multi_socket_action(curlm_, fd, CURL_POLL_OUT, &running_handles_);
        LOG_TRACE << rc << " " << running_handles_;
    } while (rc == CURLM_CALL_MULTI_PERFORM);
    checkFinish();
}

void Curl::checkFinish()
{
    if (prev_running_handles_ > running_handles_ || running_handles_ == 0)
    {
        CURLMsg* message = NULL;
        int left = 0;
        while ((message = curl_multi_info_read(curlm_, &left)) != NULL)
        {
            if (message->msg == CURLMSG_DONE)
            {
                CURL* c = message->easy_handle;
                CURLcode response = message->data.result;
                Request* request = NULL;
                curl_easy_getinfo(c, CURLINFO_PRIVATE, &request);
                assert(request->getCurl() == c);
                LOG_TRACE << request << " done";
                request->done(response);
            }
        }
    }
}

int Curl::socketCallback(CURL* c, int fd, int what, void* userp, void* socketp)
{
    Curl* curl = static_cast<Curl*>(userp);
    const char* whatstr[] = {"none", "IN", "OUT", "INOUT", "REMOVE"};
    LOG_DEBUG << "Curl::socketCallback [" << curl << "] - fd " << fd
              << " what = " << whatstr[what];
    Request* request = NULL;
    curl_easy_getinfo(c, CURLINFO_PRIVATE, &request);
    assert(request->getCurl() == c);
    if (what == CURL_POLL_REMOVE)
    {
        Channel* channel = static_cast<Channel*>(socketp);
        assert(request->getChannel() == channel);
        request->removeChannel();
        channel = NULL;
        curl_multi_assign(curl->curlm_, fd, channel);
    }
    else
    {
        Channel* channel = static_cast<Channel*>(socketp);
        if (!channel)
        {
            channel = request->setChannel(fd);
            channel->setReadCallback(boost::bind(&Curl::onRead, curl, fd));
            channel->setWriteCallback(boost::bind(&Curl::onWrite, curl, fd));
            channel->enableReading();
            curl_multi_assign(curl->curlm_, fd, channel);
            LOG_TRACE << "new channel for fd = " << fd;
        }
        assert(request->getChannel() == channel);
        // update
        if (what & CURL_POLL_OUT)
        {
            channel->enableWriting();
        }
        else
        {
            channel->disableWriting();
        }
    }
    return 0;
}

int Curl::timerCallback(CURLM* curlm, long ms, void* userp)
{
    Curl* curl = static_cast<Curl*>(userp);
    LOG_DEBUG << curl << " " << ms << " ms";
    curl->loop_->runAfter(static_cast<int>(ms) / 1000.0, boost::bind(&Curl::onTimer, curl));
    return 0;
}
