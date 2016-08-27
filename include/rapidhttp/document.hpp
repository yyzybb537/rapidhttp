#pragma once
#include "document.h"
#include <rapidhttp/util.h>
#include <algorithm>
#include <stdio.h>

namespace rapidhttp {

    template <typename StringT>
    inline THttpDocument<StringT>::THttpDocument(DocumentType type)
        : type_(type)
    {
        Reset();
#if USE_PICO
#else
        memset(&settings_, 0, sizeof(settings_));
        settings_.on_headers_complete = sOnHeadersComplete;
        settings_.on_message_complete = sOnMessageComplete;
        settings_.on_url = sOnUrl;
        settings_.on_status = sOnStatus;
        settings_.on_header_field = sOnHeaderField;
        settings_.on_header_value = sOnHeaderValue;
        settings_.on_body = sOnBody;
#endif
    }

    template <typename StringT>
    template <typename OStringT>
    void THttpDocument<StringT>::CopyTo(THttpDocument<OStringT> & clone) const
    {
#define _COPY_TO(param) \
        clone.param = this->param

        _COPY_TO(type_);
        _COPY_TO(parse_done_);
        _COPY_TO(ec_);
        _COPY_TO(parser_);
        clone.parser_.data = &clone;
        _COPY_TO(kv_state_);
        _COPY_TO(callback_header_key_cache_);
        _COPY_TO(callback_header_value_cache_);
        _COPY_TO(major_);
        _COPY_TO(minor_);
        _COPY_TO(request_method_);
        _COPY_TO(request_uri_);
        _COPY_TO(response_status_code_);
        _COPY_TO(response_status_);
        _COPY_TO(body_);

        clone.header_fields_.clear();
        clone.header_fields_.reserve(this->header_fields_.size());
        for (auto const& kv : this->header_fields_)
        {
            clone.header_fields_.emplace_back(std::pair<OStringT, OStringT>(
                        (OStringT)kv.first, (OStringT)kv.second));
        }

#undef _COPY_TO
    }

    /// ------------------- parse/generate ---------------------
    /// 流式解析
    // @buf_ref: 外部传入的缓冲区首地址, 再调用Storage前必须保证缓冲区有效且不变.
    // @len: 缓冲区长度
    // @returns：解析完成返回error_code=0, 解析一半返回error_code=1, 解析失败返回其他错误码.
    template <typename StringT>
    inline size_t THttpDocument<StringT>::PartailParse(std::string const& buf)
    {
        return PartailParse(buf.c_str(), buf.size());
    }

#if USE_PICO
#else
    template <typename StringT>
    inline size_t THttpDocument<StringT>::PartailParse(const char* buf_ref, size_t len)
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
    template <typename StringT>
    inline bool THttpDocument<StringT>::PartailParseEof()
    {
        if (ParseDone() || ParseError())
            return false;

        PartailParse("", 0);
        return ParseDone();
    }
    template <typename StringT>
    inline bool THttpDocument<StringT>::ParseDone()
    {
        return parse_done_;
    }

    template <typename StringT>
    inline int THttpDocument<StringT>::sOnHeadersComplete(http_parser *parser)
    {
        return ((THttpDocument*)parser->data)->OnHeadersComplete(parser);
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::sOnMessageComplete(http_parser *parser)
    {
        return ((THttpDocument*)parser->data)->OnMessageComplete(parser);
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::sOnUrl(http_parser *parser, const char *at, size_t length)
    {
        return ((THttpDocument*)parser->data)->OnUrl(parser, at, length);
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::sOnStatus(http_parser *parser, const char *at, size_t length)
    {
        return ((THttpDocument*)parser->data)->OnStatus(parser, at, length);
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::sOnHeaderField(http_parser *parser, const char *at, size_t length)
    {
        return ((THttpDocument*)parser->data)->OnHeaderField(parser, at, length);
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::sOnHeaderValue(http_parser *parser, const char *at, size_t length)
    {
        return ((THttpDocument*)parser->data)->OnHeaderValue(parser, at, length);
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::sOnBody(http_parser *parser, const char *at, size_t length)
    {
        return ((THttpDocument*)parser->data)->OnBody(parser, at, length);
    }

    template <typename StringT>
    inline int THttpDocument<StringT>::OnHeadersComplete(http_parser *parser)
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
    template <typename StringT>
    inline int THttpDocument<StringT>::OnMessageComplete(http_parser *parser)
    {
        parse_done_ = true;
        return 0;
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::OnUrl(http_parser *parser, const char *at, size_t length)
    {
        request_uri_.append(at, length);
        return 0;
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::OnStatus(http_parser *parser, const char *at, size_t length)
    {
        response_status_.append(at, length);
        return 0;
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::OnHeaderField(http_parser *parser, const char *at, size_t length)
    {
        if (kv_state_ == 1) {
            header_fields_.emplace_back(std::move(callback_header_key_cache_),
                    std::move(callback_header_value_cache_));
            kv_state_ = 0;
        }

        callback_header_key_cache_.append(at, length);
        return 0;
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::OnHeaderValue(http_parser *parser, const char *at, size_t length)
    {
        kv_state_ = 1;
        callback_header_value_cache_.append(at, length);
        return 0;
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::OnBody(http_parser *parser, const char *at, size_t length)
    {
        body_.append(at, length);
        return 0;
    }
#endif

    template <typename StringT>
    inline void THttpDocument<StringT>::Reset()
    {
#if USE_PICO
#else
        http_parser_init(&parser_, IsRequest() ? HTTP_REQUEST : HTTP_RESPONSE);
        parser_.data = this;
#endif

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
    template <typename StringT>
    inline std::error_code THttpDocument<StringT>::ParseError()
    {
        return ec_;
    }

    template <typename StringT>
    inline bool THttpDocument<StringT>::IsInitialized() const
    {
        if (IsRequest())
            return CheckMethod() && CheckUri() && CheckVersion();
        else
            return CheckVersion() && CheckStatusCode() && CheckStatus();
    }

    template <typename StringT>
    inline size_t THttpDocument<StringT>::ByteSize() const
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
        bytes += body_.size();
        return bytes;
    }

    template <typename StringT>
    inline bool THttpDocument<StringT>::Serialize(char *buf, size_t len)
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
            _WRITE_C_STR("HTTP/", 5);
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
        _WRITE_STRING(body_);
        size_t length = buf - ori;
        (void)length;
        return true;
#undef _WRITE_CRLF
#undef _WRITE_C_STR
#undef _WRITE_STRING
    }
    template <typename StringT>
    inline std::string THttpDocument<StringT>::SerializeAsString()
    {
        std::string s;
        size_t bytes = ByteSize();
        if (!bytes) return "";
        s.resize(bytes);
        if (!Serialize(&s[0], bytes)) return "";
        return s;
    }
    template <typename StringT>
    inline bool THttpDocument<StringT>::CheckMethod() const
    {
        return !request_method_.empty();
    }
    template <typename StringT>
    inline bool THttpDocument<StringT>::CheckUri() const
    {
        return !request_uri_.empty() && request_uri_[0] == '/';
    }
    template <typename StringT>
    inline bool THttpDocument<StringT>::CheckStatusCode() const
    {
        return response_status_code_ >= 100 && response_status_code_ < 1000;
    }
    template <typename StringT>
    inline bool THttpDocument<StringT>::CheckStatus() const
    {
        return !response_status_.empty();
    }
    template <typename StringT>
    inline bool THttpDocument<StringT>::CheckVersion() const
    {
        return major_ >= 0 && major_ <= 9 && minor_ >= 0 && minor_ <= 9;
    }
    /// --------------------------------------------------------

    /// ------------------- fields get/set ---------------------
    template <typename StringT>
    inline StringT const& THttpDocument<StringT>::GetMethod()
    {
        return request_method_;
    }
    
    template <typename StringT>
    inline void THttpDocument<StringT>::SetMethod(const char* m)
    {
        request_method_ = m;
    }
    template <typename StringT>
    inline void THttpDocument<StringT>::SetMethod(std::string const& m)
    {
        request_method_ = m;
    }
    template <typename StringT>
    inline StringT const& THttpDocument<StringT>::GetUri()
    {
        return request_uri_;
    }
    template <typename StringT>
    inline void THttpDocument<StringT>::SetUri(const char* m)
    {
        request_uri_ = m;
    }
    template <typename StringT>
    inline void THttpDocument<StringT>::SetUri(std::string const& m)
    {
        request_uri_ = m;
    }
    template <typename StringT>
    inline StringT const& THttpDocument<StringT>::GetStatus()
    {
        return response_status_;
    }
    template <typename StringT>
    inline void THttpDocument<StringT>::SetStatus(const char* m)
    {
        response_status_ = m;
    }
    template <typename StringT>
    inline void THttpDocument<StringT>::SetStatus(std::string const& m)
    {
        response_status_ = m;
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::GetStatusCode()
    {
        return response_status_code_;
    }
    template <typename StringT>
    inline void THttpDocument<StringT>::SetStatusCode(int code)
    {
        response_status_code_ = code;
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::GetMajor()
    {
        return major_;
    }
    template <typename StringT>
    inline void THttpDocument<StringT>::SetMajor(int v)
    {
        major_ = v;
    }
    template <typename StringT>
    inline int THttpDocument<StringT>::GetMinor()
    {
        return minor_;
    }
    template <typename StringT>
    inline void THttpDocument<StringT>::SetMinor(int v)
    {
        minor_ = v;
    }
    template <typename StringT>
    inline StringT const& THttpDocument<StringT>::GetField(std::string const& k)
    {
        static const string_t empty_string;
        auto it = std::find_if(header_fields_.begin(), header_fields_.end(),
                [&](std::pair<string_t, string_t> const& kv)
                {
                    return kv.first == k;
                });
        if (header_fields_.end() == it)
            return empty_string;
        else
            return it->second;
    }
    template <typename StringT>
    inline void THttpDocument<StringT>::SetField(std::string const& k, const char* m)
    {
        auto it = std::find_if(header_fields_.begin(), header_fields_.end(),
                [&](std::pair<string_t, string_t> const& kv)
                {
                    return kv.first == k;
                });
        if (header_fields_.end() == it)
            header_fields_.emplace_back(k, m);
        else
            it->second = m;
    }
    template <typename StringT>
    inline void THttpDocument<StringT>::SetField(std::string const& k, std::string const& m)
    {
        return SetField(k, m.c_str());
    }
    template <typename StringT>
    inline StringT const& THttpDocument<StringT>::GetBody()
    {
        return body_;
    }
    template <typename StringT>
    inline void THttpDocument<StringT>::SetBody(const char* m)
    {
        body_ = m;
    }
    template <typename StringT>
    inline void THttpDocument<StringT>::SetBody(std::string const& m)
    {
        body_ = m;
    }
    /// --------------------------------------------------------

    typedef THttpDocument<std::string> HttpDocument;
    typedef THttpDocument<StringRef> HttpDocumentRef;

} //namespace rapidhttp 
