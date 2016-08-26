#include <iostream>
#include <unistd.h>
#include <rapidhttp/document.h>
using namespace std;

void serialize()
{
    // 1.定义一个Document对象
    rapidhttp::HttpDocument doc(rapidhttp::Response);

    // 2.设置status/code
    doc.SetStatusCode(200);
    doc.SetStatus("OK");

    // 3.设置版本号, 不设置时默认是HTTP/1.1
    doc.SetMajor(1);
    doc.SetMinor(1);

    // 4.设置域
    doc.SetField("Server", "rapidhttp");
    doc.SetField("Connection", "close");
    doc.SetField("Content-Length", "12");

    // 5.设置body.(二进制body使用std::string设置)
    doc.SetBody("hello world!");

    // 6.获取序列化长度
    size_t bytes = doc.ByteSize();
    if (!bytes) {
        // 长度返回0表示有些字段没有正确初始化, 不允许序列化
        cout << "serialize error" << endl;
    }
    char *buf = new char[bytes];

    // 7.调用Serialize
    bool b = doc.Serialize(buf, bytes);

    // 8.判断序列化是否成功
    if (!b) {
        cout << "serialize error" << endl;
    } else {
        cout << "serialize output:\n" << std::string(buf, bytes) << endl;
    }

    delete buf;

    // 9.不在乎性能时, 也可以直接序列化成std::string, 不必关心长度.
    std::string output = doc.SerializeAsString();
    if (output.empty()) {
        // 返回空串表示有字段没有正确初始化, 不允许序列化
        cout << "serialize error" << endl;
    }
    cout << "serialize output:\n" << output << endl;
}

int main()
{
    serialize();
    return 0;
}
