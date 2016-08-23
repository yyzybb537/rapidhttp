#pragma once

#include <string>
#include <map>
#include <stdint.h>
#include <rapidhttp/constants.h>
#include <rapidhttp/stringref.h>
#include <rapidhttp/error_code.h>

namespace rapidhttp {

// Document field
//struct Field
//{
//    enum eFlags
//    {
//        e_Update        = 0x1,
//        e_IntOrString   = 0x2,  // 0: string 1: integer.
//    };
//
//    unsigned char flags;
//    StringRef key;
//    union {
//        StringRef str;
//        uint32_t integer;
//    };
//    std::string key_storage;
//    std::string value_storage;
//
//    Field() : flags(0), key(), str() {}
//    explicit Field(StringRef const& k, int i) : flags(e_IntOrString), key(k), integer(i) {}
//    explicit Field(StringRef const& k, StringRef const& s) : flags(0), key(k), str(s) {}
//
//    bool IsUpdate() { return flags & e_Update; }
//    void SetUpdate() { flags |= e_Update; }
//
//    bool IsString() { return (flags & e_IntOrString) == 0; }
//    void SetString(const char* buf)
//    {
//        flags &= ~e_IntOrString;
//        value_storage = buf;
//        str.SetString(value_storage);
//    }
//    void SetStringRef(StringRef const& strref)
//    {
//        flags &= ~e_IntOrString;
//        str = strref;
//    }
//
//    bool IsUInt() { return flags & e_IntOrString; }
//    void SetUInt(uint32_t i)
//    {
//        flags |= e_IntOrString;
//        integer = i;
//    }
//
//    StringRef& GetStringRef() { assert(IsString()); return str; }
//    uint32_t& GetInt() { assert(IsUInt()); return integer; }
//
//    size_t ByteSize() {
//        if (IsString())
//            return str.size();
//        else {
//            return UIntegerByteSize(integer);
//        }
//    }
//
//    void OnCopy(const char* origin, const char* dest)
//    {
//        key.OnCopy(origin, dest);
//        if (IsString())
//            str.OnCopy(origin, dest);
//    }
//
//    void Storage()
//    {
//        if (IsString()) {
//            if (str.c_str() != storage_.c_str()) {
//                storage_ = str.ToString();
//                str.SetString(storage_);
//            }
//        }
//    }
//};

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

    enum class eParseState
    {
        init,
        method,
        uri,
        version,
        code,
        response_str,
        fields,
        done,
    };

    explicit HttpDocument(DocumentType type) : type_(type) {}
//    HttpDocument(HttpDocument const& other);
//    HttpDocument(HttpDocument && other);
//    HttpDocument& operator=(HttpDocument const& other);
//    HttpDocument& operator=(HttpDocument && other);

    /// ------------------- parse/generate ---------------------
    /// 流式解析
    // @buf_ref: 外部传入的缓冲区首地址
    // @len: 缓冲区长度
    // @returns：解析完成返回error_code=0, 解析一半返回error_code=1, 解析失败返回其他错误码.
    size_t PartailParse(const char* buf_ref, size_t len);
    size_t PartailParse(std::string const& buf);
    void ResetPartailParse();

    // 返回解析错误码
    std::error_code ParseError();

    // 保存
//    void Storage();

    bool IsInitialized() const;

    size_t ByteSize() const;

    bool Serialize(char *buf, size_t len);
    /// --------------------------------------------------------

    /// ------------------- fields get/set ---------------------
    std::string const& GetMethod();
    void SetMethod(const char* m);
    void SetMethod(std::string const& m);

    std::string const& GetUri();
    void SetUri(const char* m);
    void SetUri(std::string const& m);

    std::string const& GetResponseString();
    void SetResponseString(const char* m);
    void SetResponseString(std::string const& m);

    int GetCode();
    void SetCode(int code);

    int GetMajor();
    void SetMajor(int v);

    int GetMinor();
    void SetMinor(int v);

    std::string const& GetField(std::string const& k);
    void SetField(std::string const& k, const char* m);
    void SetField(std::string const& k, std::string const& m);
    /// --------------------------------------------------------

    inline bool IsRequest() const { return type_ == Request; }
    inline bool IsResponse() const { return type_ == Response; }

private:
    bool ParseMethod(const char* pos, const char* last);
    bool ParseUri(const char* pos, const char* last);
    bool ParseVersion(const char* pos, const char* last);
    bool ParseCode(const char* pos, const char* last);
    bool ParseResponseStr(const char* pos, const char* last);
    bool ParseField(const char* pos, const char* last, std::string & key,
            std::string & value);

    bool CheckMethod() const;
    bool CheckUri() const;
    bool CheckCode() const;
    bool CheckResponseString() const;
    bool CheckVersion() const;

private:
//    bool update_flags_ = false;   // 是否有内容变更
//    const char* origin_str_ = "";
//    size_t origin_length_ = 0;
//    std::string storage_;

    DocumentType type_;     // 类型

    // 默认版本号: HTTP/1.1
    uint32_t major_ = 1;
    uint32_t minor_ = 1;

    eParseState parse_state_ = eParseState::init;
    std::string parse_buffer_;
    std::error_code ec_;    // 解析错状态

    std::string request_method_;
    std::string request_uri_;

    uint32_t response_code_ = 404;
    std::string response_str_;

    std::map<std::string, std::string> header_fields_;

//    std::vector<Field> special_fields_;
};

} //namespace rapidhttp 

#include "document.hpp"
