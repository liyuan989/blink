#include <example/curl/Curl.h>

#include <blink/EventLoop.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

EventLoop* g_loop = NULL;

void onData(const char* data, int len)
{
    printf("len %d\n", len);
    // string content(data, len);
    // printf("%s\n", content.c_str());
}

void done(Request* request, int code)
{
    printf("done %p %s %d\n", request, request->getEffectiveUrl(), code);
}

void done2(Request* request, int code)
{
    printf("donw2 %p %s %d %d\n", request, request->getRedirectUrl(), request->getResponseCode(), code);
}

int main(int argc, char* argv[])
{
    EventLoop loop;
    g_loop = &loop;
    loop.runAfter(30.0, boost::bind(&EventLoop::quit, &loop));
    Curl::initialize(Curl::kCurlSsl);
    Curl curl(&loop);

    RequestPtr request = curl.getUrl("http://liyuanlife.com");
    request->setDataCallback(onData);
    request->setDoneCallback(done);

    RequestPtr request2 = curl.getUrl("https://github.com");
    request2->setDataCallback(onData);
    request2->setDoneCallback(done);

    RequestPtr request3 = curl.getUrl("http://example.com");
    request3->setDataCallback(onData);
    request3->setDoneCallback(done2);

    loop.loop();
    return 0;
}
