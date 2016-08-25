#include <iostream>
#include <unistd.h>
#include <rapidhttp/document.h>
using namespace std;

void parse()
{
    static std::string c_http_request =
        "POST /uri/abc HTTP/1.1\r\n"
        "Accept: XAccept\r\n"
        "Host: domain.com\r\n"
        "User-Agent: gtest.proxy\r\n"
        "Content-Length: 3\r\n"
        "\r\nabc";

    // 1.定义一个Document对象
    rapidhttp::HttpDocument doc(rapidhttp::HttpDocument::Request);

    // 2.调用PartailParse解析数据流, 接口参数是
    // std::string
    // 或 (const char*, size_t)
    doc.PartailParse(c_http_request);

    // 3.判断解析是否出错
    if (doc.ParseError()) {
        // 打印错误描述信息
        cout << "parse error:" << doc.ParseError().message() << endl;
        return ;
    }

    // 4.判断解析是否完成
    if (doc.ParseDone()) {
        cout << "parse not done." << endl;
    }
}

void partail_parse()
{
    static std::string c_http_request =
        "POST /uri/abc HTTP/1.1\r\n"
        "Accept: XAccept\r\n"
        "Host: domain.com\r\n"
        "User-Agent: gtest.proxy\r\n"
        "Content-Length: 3\r\n"
        "\r\nabc";

    // 1.定义一个Document对象
    rapidhttp::HttpDocument doc(rapidhttp::HttpDocument::Request);

    // 2.调用PartailParse解析数据流的一部分
    int pos = 27;
    size_t bytes = doc.PartailParse(c_http_request.c_str(), pos);

    // 3.判断解析是否出错
    if (doc.ParseError()) {
        // 打印错误描述信息
        cout << "parse error:" << doc.ParseError().message() << endl;
        return ;
    }

    // 4.判断解析是否完成
    if (!doc.ParseDone()) {
        // 未完成, 继续解析剩余部分
        bytes += doc.PartailParse(c_http_request.c_str() + pos, c_http_request.size() - pos);
    }

    // 4.判断解析是否完成
    if (doc.ParseDone()) {
        cout << "parse not done." << endl;
    }
}

int main()
{
    parse();
    partail_parse();
    return 0;
}
