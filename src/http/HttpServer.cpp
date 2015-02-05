#include "http/HttpServer.h"
#include "http/HttpContext.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "Log.h"

#include <boost/bind.hpp>

namespace blink
{

void defaultHttpCallback(const HttpRequest& request, HttpResponse* response)
{
    response->setStatusCode(HttpResponse::kNotFound);
    response->setStatusMessage("Not Found");
    response->setCloseConnection(true);
}

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listen_addr,
                       const string& name,
                       TcpServer::Option option)
    : server_(loop, listen_addr, name),
      http_callback_(defaultHttpCallback)
{
    server_.setConnectionCallback(boost::bind(&HttpServer::onConnection, this, _1));
    server_.setMessageCallback(boost::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

HttpServer::~HttpServer()
{
}

void HttpServer::start()
{
    LOG_WARN << "HttpServer[" <<server_.name() << "] starts listening on "
             << server_.hostport();
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& connection)
{
    if (connection->connected())
    {
        connection->setContext(HttpContext());
    }
}

void HttpServer::onMessage(const TcpConnectionPtr& connection,
                           Buffer* buf,
                           Timestamp receive_time)
{
    HttpContext* context = boost::any_cast<HttpContext>(connection->getMutableContext());
    if (!context->parseRequest(buf, receive_time))
    {
        connection->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        connection->shutdown();
    }
    if (context->gotAll())
    {
        onRequest(connection, context->request());
        context->reset();
    }
}

void HttpServer::onRequest(const TcpConnectionPtr& connection, const HttpRequest& request)
{
    const string& header = request.getHeader("Connection");
    bool close = header == "close" ||
        (request.getVersion() == HttpRequest::kHttp10 && header != "Keep-Alive");
    HttpResponse response(close);
    http_callback_(request, &response);
    Buffer buf;
    response.appendToBuffer(&buf);
    connection->send(&buf);
    if (response.closeConnection())
    {
        connection->shutdown();
    }
}

}  // namespace blink
