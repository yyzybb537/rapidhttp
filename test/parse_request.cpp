#include <iostream>
#include <unistd.h>
#include <rapidhttp/document.h>
#include <gtest/gtest.h>
using namespace std;
using namespace rapidhttp;

static std::string c_http_request = 
"GET /uri/abc HTTP/1.1\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"Connection: Keep-Alive\r\n"
"\r\n";

static std::string c_http_request_2 = 
"POST /uri/abc HTTP/1.1\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"User-Agent: gtest.proxy\r\n"
"Content-Length: 3\r\n"
"\r\nabc";

// 错误的协议头
static std::string c_http_request_err_1 = 
"POST/uri/abc HTTP/1.1\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"User-Agent: gtest.proxy\r\n"
"\r\n";

// 兼容HTTP0.9的协议头
static std::string c_http_request_http_0_9 = 
"POST /uri/abcHTTP/1.1\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"User-Agent: gtest.proxy\r\n"
"\r\n";

// 一部分协议头, 缺少一个\r\n
static std::string c_http_request_err_3 = 
"POST /uri/abc HTTP/1.1\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"User-Agent: gtest.proxy\r\n";

TEST(parse, request)
{
    rapidhttp::HttpDocument doc(rapidhttp::HttpDocument::Request);
    size_t bytes = doc.PartailParse(c_http_request);
    EXPECT_EQ(bytes, c_http_request.size());
    EXPECT_TRUE(!doc.ParseError());

    EXPECT_EQ(doc.GetMethod(), "GET");
    EXPECT_EQ(doc.GetUri(), "/uri/abc");
    EXPECT_EQ(doc.GetMajor(), 1);
    EXPECT_EQ(doc.GetMinor(), 1);
    EXPECT_EQ(doc.GetField("Accept"), "XAccept");
    EXPECT_EQ(doc.GetField("Host"), "domain.com");
    EXPECT_EQ(doc.GetField("Connection"), "Keep-Alive");
    EXPECT_EQ(doc.GetField("User-Agent"), "");

    for (int i = 0; i < 10; ++i) {
        size_t bytes = doc.PartailParse(c_http_request.c_str(), c_http_request.size());
        EXPECT_EQ(bytes, c_http_request.size());
        EXPECT_TRUE(!doc.ParseError());
    }

    EXPECT_EQ(doc.GetMethod(), "GET");
    EXPECT_EQ(doc.GetUri(), "/uri/abc");
    EXPECT_EQ(doc.GetMajor(), 1);
    EXPECT_EQ(doc.GetMinor(), 1);
    EXPECT_EQ(doc.GetField("Accept"), "XAccept");
    EXPECT_EQ(doc.GetField("Host"), "domain.com");
    EXPECT_EQ(doc.GetField("Connection"), "Keep-Alive");
    EXPECT_EQ(doc.GetField("User-Agent"), "");

    bytes = doc.PartailParse(c_http_request_2);
    EXPECT_EQ(bytes, c_http_request_2.size());
    EXPECT_TRUE(!doc.ParseError());
    EXPECT_EQ(doc.GetMethod(), "POST");
    EXPECT_EQ(doc.GetUri(), "/uri/abc");
    EXPECT_EQ(doc.GetMajor(), 1);
    EXPECT_EQ(doc.GetMinor(), 1);
    EXPECT_EQ(doc.GetField("Accept"), "XAccept");
    EXPECT_EQ(doc.GetField("Host"), "domain.com");
    EXPECT_EQ(doc.GetField("Connection"), "");
    EXPECT_EQ(doc.GetField("User-Agent"), "gtest.proxy");
    EXPECT_EQ(doc.GetBody(), "abc");

    bytes = doc.PartailParse(c_http_request_err_1);
    EXPECT_TRUE(doc.ParseError());

    bytes = doc.PartailParse(c_http_request_http_0_9);
    EXPECT_FALSE(doc.ParseError());
    EXPECT_TRUE(doc.ParseDone());
    EXPECT_EQ(doc.GetMajor(), 0);
    EXPECT_EQ(doc.GetMinor(), 9);

    // partail parse logic
    cout << "parse partail" << endl;
    bytes = doc.PartailParse(c_http_request_err_3);
    EXPECT_EQ(bytes, c_http_request_err_3.size());
    EXPECT_FALSE(doc.ParseError());
    EXPECT_FALSE(doc.ParseDone());

    bytes = doc.PartailParse("\r\n");
    EXPECT_EQ(bytes, 2);
    EXPECT_FALSE(doc.ParseError());

    for (size_t pos = 0; pos < c_http_request.size(); ++pos)
    {
//        cout << "parse split by " << pos << endl;
        std::string fp = c_http_request.substr(0, pos);
        size_t bytes = doc.PartailParse(fp);
//        EXPECT_EQ(bytes, pos);
//        EXPECT_EQ(doc.ParseError().value(), 1);
        EXPECT_FALSE(doc.ParseDone());

        std::string sp = c_http_request.substr(bytes);
        bytes += doc.PartailParse(sp);
        EXPECT_EQ(bytes, c_http_request.size());
        EXPECT_TRUE(!doc.ParseError());
        EXPECT_TRUE(doc.ParseDone());

        EXPECT_EQ(doc.GetMethod(), "GET");
        EXPECT_EQ(doc.GetUri(), "/uri/abc");
        EXPECT_EQ(doc.GetMajor(), 1);
        EXPECT_EQ(doc.GetMinor(), 1);
        EXPECT_EQ(doc.GetField("Accept"), "XAccept");
        EXPECT_EQ(doc.GetField("Host"), "domain.com");
        EXPECT_EQ(doc.GetField("Connection"), "Keep-Alive");
        EXPECT_EQ(doc.GetField("User-Agent"), "");
    }

    char buf[256] = {};
    bool b = doc.Serialize(buf, sizeof(buf));
    EXPECT_TRUE(b);
    bytes = doc.ByteSize();
    EXPECT_EQ(bytes, c_http_request.size());
//    EXPECT_EQ(c_http_request, buf);
}
