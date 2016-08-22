#pragma once

#include "constants.h"
#include "stringref.h"
#include "error_code.h"

namespace rapidhttp {

// Document field
struct Field
{
    enum eFlags
    {
        e_Update        = 0x1,
        e_IntOrString   = 0x2,  // 0: string 1: integer.
    };

    unsigned char flags;
    union {
        StringRef str;
        int integer;
    };
    std::string storage_;

    Field() : flags(0), str() {}
    explicit Field(int i) : flags(e_IntOrString), integer(i) {}
    explicit Field(StringRef const& s) : flags(0), str(s) {}

    bool IsUpdate() { return flags & e_Update; }
    void SetUpdate() { flags |= e_Update; }

    bool IsString() { return (flags & e_IntOrString) == 0; }
    void SetString(const char* buf)
    {
        flags &= ~e_IntOrString;
        storage_ = buf;
        str.SetString(storage_);
    }
    void SetStringRef(StringRef const& strref)
    {
        flags &= ~e_IntOrString;
        str = strref;
    }

    bool IsInt() { return flags & e_IntOrString; }
    void SetInt(int i)
    {
        flags |= e_IntOrString;
        integer = i;
    }

    StringRef& GetStringRef() { assert(IsString()); return str; }
    int& GetInt() { assert(IsInt()); return integer; }
};

// Http document class.
// Copy-on-write.
class HttpDocument
{
public:
    enum DocumentType
    {
        Request,
        Response,
    };
    // 协议头
    // e.g: GET /uri HTTP/1.1
    // or
    // e.g: HTTP/1.1 200 OK
    enum eProtoFieldsIndex
    {
        e_method = 0,       // 方法 @Request
        e_response_str = 0, // URI @Request
        e_uri = 1,          // 响应字符串 @Resposne
        e_code = 1,         // 响应Code @Resposne
        e_ver_major = 2,    // 协议主版本号
        e_ver_minor = 3,    // 协议次版本号
        e_proto_fields_count = 4,
    };
    // 常用头部域
    enum eHeaderFields
    {
        e_Host = e_proto_fields_count,
        e_Accept,
        e_User_Agent,
        e_Authorization,
        e_Expect,
        e_From,
        e_Proxy_Authorization,
        e_Referer,
        e_Range,
        e_Accept_Charset,
        e_Accept_Encoding,
        e_Accept_Language,
        e_Accept_Range,
        e_Max_Forwards,
        e_header_fields_count,
    };

    explicit HttpDocument(DocumentType type) : type_(type) {}
    HttpDocument(HttpDocument const& other);
    HttpDocument(HttpDocument && other);
    HttpDocument& operator=(HttpDocument const& other);
    HttpDocument& operator=(HttpDocument && other);

    /// ------------------- parse/generate ---------------------
    /// 流式解析
    // @buf_ref: 外部传入的缓冲区首地址, 再调用Storage前必须保证缓冲区有效且不变.
    // @len: 缓冲区长度
    // @returns：解析完成返回error_code=0, 解析一半返回error_code=1, 解析失败返回其他错误码.
    std::error_code PartailParse(const char* buf_ref, size_t len);

    // 返回解析错误码
    std::error_code ParseError();

    // 保存
    void Storage();

    size_t ByteSize();

    bool Write(char *buf_ref, size_t len);
    /// --------------------------------------------------------

    /// ------------------- fields get/set ---------------------
    StringRef GetMethod();
    
    void SetMethodRef(const char* buf_ref);

    void SetMethod(const char* buf);
    /// --------------------------------------------------------

private:
    void OnCopy();

private:
    uint32_t update_flags_ = 0;   // 是否有内容变更
    std::error_code ec_;    // 解析错状态

    const char* origin_str_ = "";
    size_t origin_length_ = 0;
    std::string storage_;

    DocumentType type_;     // 类型

    Field[e_header_fields_count] common_fields_;

    std::vector<Field> special_fields_;
};

} //namespace rapidhttp 

#include "document.hpp"
