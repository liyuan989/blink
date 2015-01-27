#include "http/HttpRequest.h"
#include "http/HttpContext.h"
#include "Buffer.h"

#include <gtest/gtest.h>

using namespace blink;

TEST(testParseRequest, AllInOne)
{
    HttpContext context;
    Buffer input;
    input.append("GET /index.html HTTP/1.1\r\n"
                 "Host: www.google.com\r\n"
                 "\r\n");
    EXPECT_TRUE(context.parseRequest(&input, Timestamp::now()));
    EXPECT_TRUE(context.gotAll());

    const HttpRequest& request = context.request();
    EXPECT_EQ(HttpRequest::kGet, request.method());
    EXPECT_EQ(std::string("/index.html"), request.path());
    EXPECT_EQ(HttpRequest::kHttp11, request.getVersion());
    EXPECT_EQ(std::string("www.google.com"), request.getHeader("Host"));
    EXPECT_EQ(std::string(""), request.getHeader("User-Agent"));
}

TEST(testParseRequest, InTwoPieces)
{
    std::string str("GET /index.html HTTP/1.1\r\n"
                    "Host: www.google.com\r\n"
                    "\r\n");
    for (size_t i = 0; i < str.size(); ++i)
    {
        HttpContext context;
        Buffer input;
        input.append(str.c_str(), i);
        EXPECT_TRUE(context.parseRequest(&input, Timestamp::now()));
        EXPECT_TRUE(!context.gotAll());

        size_t len = str.size() - i;
        input.append(str.c_str() + i, len);
        EXPECT_TRUE(context.parseRequest(&input, Timestamp::now()));
        EXPECT_TRUE(context.gotAll());

        const HttpRequest& request = context.request();
        EXPECT_EQ(HttpRequest::kGet, request.method());
        EXPECT_EQ(std::string("/index.html"), request.path());
        EXPECT_EQ(HttpRequest::kHttp11, request.getVersion());
        EXPECT_EQ(std::string("www.google.com"), request.getHeader("Host"));
        EXPECT_EQ(std::string(""), request.getHeader("User-Agent"));
    }
}

TEST(testParseRequest, EmplyHeaderValue)
{
    HttpContext context;
    Buffer input;
    input.append("GET /index.html HTTP/1.1\r\n"
                 "Host: www.google.com\r\n"
                 "User-Agent:\r\n"
                 "Accept-Encoding: \r\n"
                 "\r\n");
    EXPECT_TRUE(context.parseRequest(&input, Timestamp::now()));
    EXPECT_TRUE(context.gotAll());

    const HttpRequest& request = context.request();
    EXPECT_EQ(HttpRequest::kGet, request.method());
    EXPECT_EQ(std::string("/index.html"), request.path());
    EXPECT_EQ(HttpRequest::kHttp11, request.getVersion());
    EXPECT_EQ(std::string("www.google.com"), request.getHeader("Host"));
    EXPECT_EQ(std::string(""), request.getHeader("User-Agent"));
    EXPECT_EQ(std::string(""), request.getHeader("Accept-Encoding"));
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    (void)ret;
}
