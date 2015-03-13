#ifndef __BLINK_HTTP_HTTPSERVER_H__
#define __BLINK_HTTP_HTTPSERVER_H__

#include <blink/Nocopyable.h>
#include <blink/TcpServer.h>

namespace blink
{

class HttpRequest;
class HttpResponse;

// A simple embeddable HTTP server.
// It is not a fully HTTP 1.1 comliant server, but provides minimum features
// that can communicate with HTTP client and web browser.
class HttpServer : Nocopyable
{
public:
    typedef boost::function<void (const HttpRequest&, HttpResponse*)> HttpCallback;

    HttpServer(EventLoop* loop,
               const InetAddress& listen_addr,
               const string& name,
               TcpServer::Option option = TcpServer::kNoReusePort);

    // force out-line destructor, for scoped_ptr member.
    ~HttpServer();

    void start();

    EventLoop* getLoop() const
    {
        return server_.getLoop();
    }

    // Not thread safe, callback should be registered before calling start().
    void setHttpCallback(const HttpCallback& cb)
    {
        http_callback_ = cb;
    }

    void setThreadNumber(int number_threads)
    {
        server_.setThreadNumber(number_threads);
    }

private:
    void onConnection(const TcpConnectionPtr& connection);
    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time);
    void onRequest(const TcpConnectionPtr& connection, const HttpRequest& request);

    TcpServer     server_;
    HttpCallback  http_callback_;
};

}  // namespace blink

#endif
