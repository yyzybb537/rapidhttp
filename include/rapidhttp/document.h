#pragma once

#include <string>
#include <map>
#include <vector>
#include <stdint.h>
#include <rapidhttp/constants.h>
#include <rapidhttp/stringref.h>
#include <rapidhttp/error_code.h>
#include <rapidhttp/http_parser.hpp>

namespace rapidhttp {

enum DocumentType
{
    Request,
    Response,
};

// Http Header document class.
template <typename StringT>
class THttpDocument
{
public:
    typedef StringT string_t;

    explicit THttpDocument(DocumentType type);
    THttpDocument(THttpDocument const& other) = delete;
    THttpDocument(THttpDocument && other) = delete;
    THttpDocument& operator=(THttpDocument const& other) = delete;
    THttpDocument& operator=(THttpDocument && other) = delete;

    template <typename OStringT>
    void CopyTo(THttpDocument<OStringT> & clone) const;

    /// ------------------- parse/generate ---------------------
    /// 流式解析
    // @buf_ref: 外部传入的缓冲区首地址
    // @len: 缓冲区长度
    // @returns：返回已成功解析到的数据长度
    inline size_t PartailParse(const char* buf_ref, size_t len);
    inline size_t PartailParse(std::string const& buf);

    /// 解析eof 
    // 解析Response时, 断开链接时要调用这个接口, 因为有些response协议需要读取到
    // 网络链接断开为止.
    inline bool PartailParseEof();

    /// 是否解析成功
    inline bool ParseDone();

    /// 重置解析流状态
    // 同时清除解析流状态和已解析成功的数据状态
    inline void Reset();

    /// 返回解析错误码
    inline std::error_code ParseError();

    /// 是否全部初始化完成, Serialize之前会做这个校验
    inline bool IsInitialized() const;

    /// Serialize后的数据长度
    inline size_t ByteSize() const;

    /// 序列化
    inline bool Serialize(char *buf, size_t len);
    inline std::string SerializeAsString();
    /// --------------------------------------------------------

    /// ------------------- fields get/set ---------------------
    inline string_t const& GetMethod();
    inline void SetMethod(const char* m);
    inline void SetMethod(std::string const& m);

    inline string_t const& GetUri();
    inline void SetUri(const char* m);
    inline void SetUri(std::string const& m);

    inline string_t const& GetStatus();
    inline void SetStatus(const char* m);
    inline void SetStatus(std::string const& m);

    inline int GetStatusCode();
    inline void SetStatusCode(int code);

    inline int GetMajor();
    inline void SetMajor(int v);

    inline int GetMinor();
    inline void SetMinor(int v);

    inline string_t const& GetField(std::string const& k);
    inline void SetField(std::string const& k, const char* m);
    inline void SetField(std::string const& k, std::string const& m);

    inline string_t const& GetBody();
    inline void SetBody(const char* m);
    inline void SetBody(std::string const& m);
    /// --------------------------------------------------------

    inline bool IsRequest() const { return type_ == Request; }
    inline bool IsResponse() const { return type_ == Response; }

private:
    inline bool CheckMethod() const;
    inline bool CheckUri() const;
    inline bool CheckStatusCode() const;
    inline bool CheckStatus() const;
    inline bool CheckVersion() const;

    // http-parser
private:
    static inline int sOnHeadersComplete(http_parser *parser);
    static inline int sOnMessageComplete(http_parser *parser);
    static inline int sOnUrl(http_parser *parser, const char *at, size_t length);
    static inline int sOnStatus(http_parser *parser, const char *at, size_t length);
    static inline int sOnHeaderField(http_parser *parser, const char *at, size_t length);
    static inline int sOnHeaderValue(http_parser *parser, const char *at, size_t length);
    static inline int sOnBody(http_parser *parser, const char *at, size_t length);

    inline int OnHeadersComplete(http_parser *parser);
    inline int OnMessageComplete(http_parser *parser);
    inline int OnUrl(http_parser *parser, const char *at, size_t length);
    inline int OnStatus(http_parser *parser, const char *at, size_t length);
    inline int OnHeaderField(http_parser *parser, const char *at, size_t length);
    inline int OnHeaderValue(http_parser *parser, const char *at, size_t length);
    inline int OnBody(http_parser *parser, const char *at, size_t length);

private:
    DocumentType type_;     // 类型

    bool parse_done_ = false;
    std::error_code ec_;    // 解析错状态

    struct http_parser parser_;
    struct http_parser_settings settings_;
    int kv_state_ = 0;
    string_t callback_header_key_cache_;
    string_t callback_header_value_cache_;

    // 默认版本号: HTTP/1.1
    uint32_t major_ = 1;
    uint32_t minor_ = 1;

    string_t request_method_;
    string_t request_uri_;

    uint32_t response_status_code_ = 0;
    string_t response_status_;

    std::vector<std::pair<string_t, string_t>> header_fields_;

    string_t body_;

    template <typename T>
    friend class THttpDocument;
};

} //namespace rapidhttp 

#include "document.hpp"
