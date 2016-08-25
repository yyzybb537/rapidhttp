#pragma once
#include "document.h"
#include <rapidhttp/util.h>
#include <algorithm>
#include <stdio.h>

namespace rapidhttp {
    inline HttpDocument::HttpDocument(DocumentType type)
        : type_(type)
    {
        Reset();
        memset(&settings_, 0, sizeof(settings_));
        settings_.on_headers_complete = sOnHeadersComplete;
        settings_.on_message_complete = sOnMessageComplete;
        settings_.on_url = sOnUrl;
        settings_.on_status = sOnStatus;
        settings_.on_header_field = sOnHeaderField;
        settings_.on_header_value = sOnHeaderValue;
        settings_.on_body = sOnBody;
    }

    /// ------------------- parse/generate ---------------------
    /// 流式解析
    // @buf_ref: 外部传入的缓冲区首地址, 再调用Storage前必须保证缓冲区有效且不变.
    // @len: 缓冲区长度
    // @returns：解析完成返回error_code=0, 解析一半返回error_code=1, 解析失败返回其他错误码.
    inline size_t HttpDocument::PartailParse(std::string const& buf)
    {
        return PartailParse(buf.c_str(), buf.size());
    }
    inline size_t HttpDocument::PartailParse(const char* buf_ref, size_t len)
    {
        if (ParseDone() || ParseError())
            Reset();

        size_t parsed = http_parser_execute(&parser_, &settings_, buf_ref, len);
        if (parser_.http_errno) {
            // TODO: support pause
            ec_ = MakeParseErrorCode(parser_.http_errno);
        }
        return parsed;
    }
    inline bool HttpDocument::PartailParseEof()
    {
        if (ParseDone() || ParseError())
            return false;

        PartailParse("", 0);
        return ParseDone();
    }
    inline bool HttpDocument::ParseDone()
    {
        return parse_done_;
    }

    inline int HttpDocument::sOnHeadersComplete(http_parser *parser)
    {
        return ((HttpDocument*)parser->data)->OnHeadersComplete(parser);
    }
    inline int HttpDocument::sOnMessageComplete(http_parser *parser)
    {
        return ((HttpDocument*)parser->data)->OnMessageComplete(parser);
    }
    inline int HttpDocument::sOnUrl(http_parser *parser, const char *at, size_t length)
    {
        return ((HttpDocument*)parser->data)->OnUrl(parser, at, length);
    }
    inline int HttpDocument::sOnStatus(http_parser *parser, const char *at, size_t length)
    {
        return ((HttpDocument*)parser->data)->OnStatus(parser, at, length);
    }
    inline int HttpDocument::sOnHeaderField(http_parser *parser, const char *at, size_t length)
    {
        return ((HttpDocument*)parser->data)->OnHeaderField(parser, at, length);
    }
    inline int HttpDocument::sOnHeaderValue(http_parser *parser, const char *at, size_t length)
    {
        return ((HttpDocument*)parser->data)->OnHeaderValue(parser, at, length);
    }
    inline int HttpDocument::sOnBody(http_parser *parser, const char *at, size_t length)
    {
        return ((HttpDocument*)parser->data)->OnBody(parser, at, length);
    }

    inline int HttpDocument::OnHeadersComplete(http_parser *parser)
    {
        if (IsRequest())
            request_method_ = http_method_str((http_method)parser->method);
        else
            response_status_code_ = parser->status_code;
        major_ = parser->http_major;
        minor_ = parser->http_minor;
        if (kv_state_ == 1) {
            header_fields_.emplace_back(std::move(callback_header_key_cache_),
                    std::move(callback_header_value_cache_));
            kv_state_ = 0;
        }
        return 0;
    }
    inline int HttpDocument::OnMessageComplete(http_parser *parser)
    {
        parse_done_ = true;
        return 0;
    }
    inline int HttpDocument::OnUrl(http_parser *parser, const char *at, size_t length)
    {
        request_uri_.append(at, length);
        return 0;
    }
    inline int HttpDocument::OnStatus(http_parser *parser, const char *at, size_t length)
    {
        response_status_.append(at, length);
        return 0;
    }
    inline int HttpDocument::OnHeaderField(http_parser *parser, const char *at, size_t length)
    {
        if (kv_state_ == 1) {
            header_fields_.emplace_back(std::move(callback_header_key_cache_),
                    std::move(callback_header_value_cache_));
            kv_state_ = 0;
        }

        callback_header_key_cache_.append(at, length);
        return 0;
    }
    inline int HttpDocument::OnHeaderValue(http_parser *parser, const char *at, size_t length)
    {
        kv_state_ = 1;
        callback_header_value_cache_.append(at, length);
        return 0;
    }
    inline int HttpDocument::OnBody(http_parser *parser, const char *at, size_t length)
    {
        body_.append(at, length);
        return 0;
    }

    inline void HttpDocument::Reset()
    {
        http_parser_init(&parser_, IsRequest() ? HTTP_REQUEST : HTTP_RESPONSE);
        parser_.data = this;
        parse_done_ = false;
        ec_ = std::error_code();
        kv_state_ = 0;
        callback_header_key_cache_.clear();
        callback_header_value_cache_.clear();
        major_ = 1;
        minor_ = 1;
        request_method_.clear();
        request_uri_.clear();
        response_status_code_ = 0;
        response_status_.clear();
        header_fields_.clear();
        body_.clear();
    }

    // 返回解析错误码
    inline std::error_code HttpDocument::ParseError()
    {
        return ec_;
    }

    inline bool HttpDocument::IsInitialized() const
    {
        if (IsRequest())
            return CheckMethod() && CheckUri() && CheckVersion();
        else
            return CheckVersion() && CheckStatusCode() && CheckStatus();
    }

    inline size_t HttpDocument::ByteSize() const
    {
        if (!IsInitialized()) return 0;

        size_t bytes = 0;
        if (IsRequest()) {
            bytes += request_method_.size() + 1; // GET\s
            bytes += request_uri_.size() + 1;   // /uri\s
            bytes += 10;    // HTTP/1.1CRLF
        } else {
            bytes += 9;     // HTTP/1.1\s
            bytes += UIntegerByteSize(response_status_code_) + 1;  // 200\s
            bytes += response_status_.size() + 2;  // okCRLF
        }
        for (auto const& kv : header_fields_) {
            bytes += kv.first.size() + 2 + kv.second.size() + 2;
        }
        bytes += 2;
        return bytes;
    }

    inline bool HttpDocument::Serialize(char *buf, size_t len)
    {
        size_t bytes = ByteSize();
        if (!bytes || len < bytes) return false;
#define _WRITE_STRING(ss) \
        do {\
            memcpy(buf, ss.c_str(), ss.size()); \
            buf += ss.size(); \
        } while(0);

#define _WRITE_C_STR(c_str, length) \
        do {\
            memcpy(buf, c_str, length); \
            buf += length; \
        } while(0);

#define _WRITE_CRLF() \
        *buf++ = '\r';\
        *buf++ = '\n'

        char *ori = buf;
        if (IsRequest()) {
            _WRITE_STRING(request_method_);
            *buf++ = ' ';
            _WRITE_STRING(request_uri_);
            _WRITE_C_STR(" HTTP/", 6);
            *buf++ = major_ + '0';
            *buf++ = '.';
            *buf++ = minor_ + '0';
        } else {
            _WRITE_C_STR("HTTP/", 6);
            *buf++ = major_ + '0';
            *buf++ = '.';
            *buf++ = minor_ + '0';
            *buf++ = ' ';
            *buf++ = (response_status_code_ / 100) + '0';
            *buf++ = (response_status_code_ % 100) / 10 + '0';
            *buf++ = (response_status_code_ % 10) + '0';
            *buf++ = ' ';
            _WRITE_STRING(response_status_);
        }
        _WRITE_CRLF();
        for (auto const& kv : header_fields_) {
            _WRITE_STRING(kv.first);
            *buf++ = ':';
            *buf++ = ' ';
            _WRITE_STRING(kv.second);
            _WRITE_CRLF();
        }
        _WRITE_CRLF();
        size_t length = buf - ori;
        (void)length;
        return true;
#undef _WRITE_CRLF
#undef _WRITE_C_STR
#undef _WRITE_STRING
    }
    inline bool HttpDocument::CheckMethod() const
    {
        return !request_method_.empty();
    }
    inline bool HttpDocument::CheckUri() const
    {
        return !request_uri_.empty() && request_uri_[0] == '/';
    }
    inline bool HttpDocument::CheckStatusCode() const
    {
        return response_status_code_ >= 100 && response_status_code_ < 1000;
    }
    inline bool HttpDocument::CheckStatus() const
    {
        return true;
    }
    inline bool HttpDocument::CheckVersion() const
    {
        return major_ >= 0 && major_ <= 9 && minor_ >= 0 && minor_ <= 9;
    }
    /// --------------------------------------------------------

    /// ------------------- fields get/set ---------------------
    inline std::string const& HttpDocument::GetMethod()
    {
        return request_method_;
    }
    
    inline void HttpDocument::SetMethod(const char* m)
    {
        request_method_ = m;
    }
    inline void HttpDocument::SetMethod(std::string const& m)
    {
        request_method_ = m;
    }
    inline std::string const& HttpDocument::GetUri()
    {
        return request_uri_;
    }
    inline void HttpDocument::SetUri(const char* m)
    {
        request_uri_ = m;
    }
    inline void HttpDocument::SetUri(std::string const& m)
    {
        request_uri_ = m;
    }
    inline std::string const& HttpDocument::GetStatus()
    {
        return response_status_;
    }
    inline void HttpDocument::SetStatus(const char* m)
    {
        response_status_ = m;
    }
    inline void HttpDocument::SetStatus(std::string const& m)
    {
        response_status_ = m;
    }
    inline int HttpDocument::GetStatusCode()
    {
        return response_status_code_;
    }
    inline void HttpDocument::SetStatusCode(int code)
    {
        response_status_code_ = code;
    }
    inline int HttpDocument::GetMajor()
    {
        return major_;
    }
    inline void HttpDocument::SetMajor(int v)
    {
        major_ = v;
    }
    inline int HttpDocument::GetMinor()
    {
        return minor_;
    }
    inline void HttpDocument::SetMinor(int v)
    {
        minor_ = v;
    }
    inline std::string const& HttpDocument::GetField(std::string const& k)
    {
        static const std::string empty_string = "";
        auto it = std::find_if(header_fields_.begin(), header_fields_.end(),
                [&](std::pair<std::string, std::string> const& kv)
                {
                    return kv.first == k;
                });
        if (header_fields_.end() == it)
            return empty_string;
        else
            return it->second;
    }
    inline void HttpDocument::SetField(std::string const& k, const char* m)
    {
        auto it = std::find_if(header_fields_.begin(), header_fields_.end(),
                [&](std::pair<std::string, std::string> const& kv)
                {
                    return kv.first == k;
                });
        if (header_fields_.end() == it)
            header_fields_.emplace_back(k, m);
        else
            it->second = m;
    }
    inline void HttpDocument::SetField(std::string const& k, std::string const& m)
    {
        return SetField(k, m.c_str());
    }
    inline std::string const& HttpDocument::GetBody()
    {
        return body_;
    }
    inline void HttpDocument::SetBody(const char* m)
    {
        body_ = m;
    }
    inline void HttpDocument::SetBody(std::string const& m)
    {
        body_ = m;
    }
    /// --------------------------------------------------------

} //namespace rapidhttp 
