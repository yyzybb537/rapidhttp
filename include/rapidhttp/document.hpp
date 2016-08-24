#pragma once
#include "document.h"
#include <rapidhttp/util.h>
#include <algorithm>

namespace rapidhttp {
//    HttpHeaderDocument::HttpHeaderDocument(HttpHeaderDocument const& other);
//    HttpHeaderDocument::HttpHeaderDocument(HttpHeaderDocument && other);
//    HttpHeaderDocument& HttpHeaderDocument::operator=(HttpHeaderDocument const& other);
//    HttpHeaderDocument& HttpHeaderDocument::operator=(HttpHeaderDocument && other);

    /// ------------------- parse/generate ---------------------
    /// 流式解析
    // @buf_ref: 外部传入的缓冲区首地址, 再调用Storage前必须保证缓冲区有效且不变.
    // @len: 缓冲区长度
    // @returns：解析完成返回error_code=0, 解析一半返回error_code=1, 解析失败返回其他错误码.
    inline size_t HttpHeaderDocument::PartailParse(std::string const& buf)
    {
        return PartailParse(buf.c_str(), buf.size());
    }
    inline size_t HttpHeaderDocument::PartailParse(const char* buf_ref, size_t len)
    {
        size_t remain_length = 0;
        if (ec_.value() == (int)eErrorCode::parse_error ||
                parse_state_ == eParseState::done) {
            ResetPartailParse();
        } else if (ec_.value() == (int)eErrorCode::parse_progress) {
            if (!parse_buffer_.empty()) {
                remain_length = parse_buffer_.length();
                parse_buffer_.append(buf_ref, len);
                buf_ref = parse_buffer_.c_str();
                len = parse_buffer_.size();
            }
            ec_.clear();
        }

        const char* pos = buf_ref;
        const char* last = buf_ref + len;

#define _CHECK_PROGRESS(next) \
        do { \
            if (!next) { \
                parse_buffer_.assign(pos, last); \
                ec_ = MakeErrorCode(eErrorCode::parse_progress); \
                return len; \
            } \
        } while (0)

#define _CHECK_ERRORCODE() \
        do { \
            if (ec_) return 0; \
        } while (0)

        // status line.
        for (;;) {
            if (pos == last) {
                ec_ = MakeErrorCode(eErrorCode::parse_progress);
                return last - buf_ref - remain_length;
            }

            switch (parse_state_) {
                case eParseState::init:
                    {
                        pos = SkipSpaces(pos, last);
                        if (IsRequest())
                            parse_state_ = eParseState::method;
                        else {
                            parse_state_ = eParseState::version;
                            break;
                        }
                    }
//                    break;

                case eParseState::method:
                    {
                        const char* space = FindSpaces(pos, last);
                        _CHECK_PROGRESS(space);

                        if (!ParseMethod(pos, space)) {
                            ec_ = MakeErrorCode(eErrorCode::parse_error);
                            return 0;
                        }
                        pos = space + 1;
                        parse_state_ = eParseState::uri;
                    }
//                    break;

                case eParseState::uri:
                    {
                        const char* space = FindSpaces(pos, last);
                        _CHECK_PROGRESS(space);

                        if (!ParseUri(pos, space)) {
                            ec_ = MakeErrorCode(eErrorCode::parse_error);
                            return 0;
                        }
                        pos = space + 1;
                        parse_state_ = eParseState::version;
                    }
//                    break;

                case eParseState::version:
                    {
                        const char* next = nullptr;
                        if (IsRequest())
                            next = FindCRLF(pos, last, ec_);
                        else
                            next = FindSpaces(pos, last);

                        _CHECK_ERRORCODE();
                        _CHECK_PROGRESS(next);

                        if (!ParseVersion(pos, next)) {
                            ec_ = MakeErrorCode(eErrorCode::parse_error);
                            return 0;
                        }
                        pos = (IsRequest()) ? next + 2 : next + 1;
                        parse_state_ = (IsRequest()) ? eParseState::fields : eParseState::code;
                        if (IsRequest()) {
                            parse_state_ = eParseState::fields;
                            break;
                        } else
                            parse_state_ = eParseState::code;
                    }
//                    break;

                case eParseState::code:
                    {
                        const char* space = FindSpaces(pos, last);
                        _CHECK_PROGRESS(space);

                        if (!ParseCode(pos, space)) {
                            ec_ = MakeErrorCode(eErrorCode::parse_error);
                            return 0;
                        }
                        pos = space + 1;
                        parse_state_ = eParseState::response_str;
                    }
//                    break;

                case eParseState::response_str:
                    {
                        const char* next = FindCRLF(pos, last, ec_);
                        _CHECK_ERRORCODE();
                        _CHECK_PROGRESS(next);

                        if (!ParseResponseStr(pos, next)) {
                            ec_ = MakeErrorCode(eErrorCode::parse_error);
                            return 0;
                        }
                        pos = next + 2;
                        parse_state_ = eParseState::fields;
                    }
//                    break;

                case eParseState::fields:
                    for (;;) {
                        if (*pos == '\r') {
                            if (pos >= last - 1) {
                                _CHECK_PROGRESS(nullptr);
                            }

                            if (*(pos + 1) == '\n') {
                                parse_state_ = eParseState::done;
                                parse_buffer_.clear();
                                pos += 2;
                                return pos - buf_ref - remain_length;
                            } else {
                                ec_ = MakeErrorCode(eErrorCode::parse_error);
                                return 0;
                            }
                        }

                        const char* next = ParseField(pos, last);
                        _CHECK_ERRORCODE();
                        _CHECK_PROGRESS(next);

                        pos = next;
                    }
                    break;

                case eParseState::done:
                default:
                    break;
            }
        }
#undef _CHECK_ERRORCODE
#undef _CHECK_PROGRESS
        return 0;
    }
    inline void HttpHeaderDocument::ResetPartailParse()
    {
        parse_state_ = eParseState::init;
        ec_ = std::error_code();
        parse_buffer_.clear();
        header_fields_.clear();
    }

    inline bool HttpHeaderDocument::ParseMethod(const char* pos, const char* last)
    {
        request_method_.assign(pos, last);
        if (!CheckMethod()) return false;
        return true;
    }
    inline bool HttpHeaderDocument::ParseUri(const char* pos, const char* last)
    {
        request_uri_.assign(pos, last);
        if (!CheckUri()) return false;
        return true;
    }
    inline bool HttpHeaderDocument::ParseVersion(const char* pos, const char* last)
    {
        static const char* sc_http = "HTTP";
        if (*(int*)pos != *(int*)sc_http) return false;
        pos += 4;
        if (*pos++ != '/') return false;
        major_ = *pos++ - '0';
        if (*pos++ != '.') return false;
        minor_ = *pos++ - '0';
        if (!CheckVersion()) return false;
        return true;
    }
    inline bool HttpHeaderDocument::ParseCode(const char* pos, const char* last)
    {
        if (last - pos != 3) return false;
        if (*pos < '1' || *pos > '9') return false;
        response_code_ = (*pos++ - '0') * 100;
        if (*pos < '0' || *pos > '9') return false;
        response_code_ += (*pos++ - '0') * 10;
        if (*pos < '0' || *pos > '9') return false;
        response_code_ += (*pos++ - '0');
        return true;
    }
    inline bool HttpHeaderDocument::ParseResponseStr(const char* pos, const char* last)
    {
        response_str_.assign(pos, last);
        if (!CheckResponseString()) return false;
        return true;
    }
    inline const char* HttpHeaderDocument::ParseField(const char* pos, const char* last)
    {
        const char* split = pos;
        split = (const char*)memchr(pos, ':', last - pos);
        if (!split || split >= last - 4) return nullptr;  // sizeof(": \r\n") == 4
        const char* key_start = pos;
        const char* key_end = split;
        ++split;
        if (*split++ != ' ') {
            ec_ = MakeErrorCode(eErrorCode::parse_error);
            return nullptr; // Must be split by ":\s"
        }
        const char* value_start = split;
        split = (const char*)memchr(split, '\r', last - split);
        if (!split || split >= last - 1) return nullptr;
        const char* value_end = split;
        ++split;
        if (*split++ != '\n') {
            ec_ = MakeErrorCode(eErrorCode::parse_error);
            return nullptr; // Must end of \r\n
        }

        header_fields_.emplace_back(std::string(key_start, key_end), std::string(value_start, value_end));
        return split;
    }

    // 返回解析错误码
    inline std::error_code HttpHeaderDocument::ParseError()
    {
        return ec_;
    }

    // 保存
//    void HttpHeaderDocument::Storage()
//    {
//        if (!update_flags_) {
//            if (origin_str_ == storage_.c_str())
//                return ;
//
//            storage_.assign(origin_str_, origin_length_);
//            for (Field & f : special_fields_)
//                f.Storage();
//        } else {
//            // save one by one.
//            std::string buf(ByteSize());
//            Write(&buf[0], buf.length());
//            for (Field & f : common_fields_)
//                f.Storage();
//            for (Field & f : special_fields_)
//                f.Storage();
//            origin_str_ = buf.c_str();
//            origin_length_ = buf.length();
//            buf.swap(storage_);
//            update_flags_ = false;
//        }
//    }
    inline bool HttpHeaderDocument::IsInitialized() const
    {
        if (IsRequest())
            return CheckMethod() && CheckUri() && CheckVersion();
        else
            return CheckVersion() && CheckCode() && CheckResponseString();
    }

    inline size_t HttpHeaderDocument::ByteSize() const
    {
        if (!IsInitialized()) return 0;

        size_t bytes = 0;
        if (IsRequest()) {
            bytes += request_method_.size() + 1; // GET\s
            bytes += request_uri_.size() + 1;   // /uri\s
            bytes += 10;    // HTTP/1.1CRLF
        } else {
            bytes += 9;     // HTTP/1.1\s
            bytes += UIntegerByteSize(response_code_) + 1;  // 200\s
            bytes += response_str_.size() + 2;  // okCRLF
        }
        for (auto const& kv : header_fields_) {
            bytes += kv.first.size() + 2 + kv.second.size() + 2;
        }
        bytes += 2;
        return bytes;
    }

    inline bool HttpHeaderDocument::Serialize(char *buf, size_t len)
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
            *buf++ = (response_code_ / 100) + '0';
            *buf++ = (response_code_ % 100) / 10 + '0';
            *buf++ = (response_code_ % 10) + '0';
            *buf++ = ' ';
            _WRITE_STRING(response_str_);
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
    inline bool HttpHeaderDocument::CheckMethod() const
    {
        return !request_method_.empty();
    }
    inline bool HttpHeaderDocument::CheckUri() const
    {
        return !request_uri_.empty() && request_uri_[0] == '/';
    }
    inline bool HttpHeaderDocument::CheckCode() const
    {
        return response_code_ >= 100 && response_code_ < 1000;
    }
    inline bool HttpHeaderDocument::CheckResponseString() const
    {
        return !response_str_.empty();
    }
    inline bool HttpHeaderDocument::CheckVersion() const
    {
        return major_ >= 0 && major_ <= 9 && minor_ >= 0 && minor_ <= 9;
    }
    /// --------------------------------------------------------

    /// ------------------- fields get/set ---------------------
    inline std::string const& HttpHeaderDocument::GetMethod()
    {
        return request_method_;
    }
    
    inline void HttpHeaderDocument::SetMethod(const char* m)
    {
        request_method_ = m;
    }
    inline void HttpHeaderDocument::SetMethod(std::string const& m)
    {
        request_method_ = m;
    }
    inline std::string const& HttpHeaderDocument::GetUri()
    {
        return request_uri_;
    }
    inline void HttpHeaderDocument::SetUri(const char* m)
    {
        request_uri_ = m;
    }
    inline void HttpHeaderDocument::SetUri(std::string const& m)
    {
        request_uri_ = m;
    }
    inline std::string const& HttpHeaderDocument::GetResponseString()
    {
        return response_str_;
    }
    inline void HttpHeaderDocument::SetResponseString(const char* m)
    {
        response_str_ = m;
    }
    inline void HttpHeaderDocument::SetResponseString(std::string const& m)
    {
        response_str_ = m;
    }
    inline int HttpHeaderDocument::GetCode()
    {
        return response_code_;
    }
    inline void HttpHeaderDocument::SetCode(int code)
    {
        response_code_ = code;
    }
    inline int HttpHeaderDocument::GetMajor()
    {
        return major_;
    }
    inline void HttpHeaderDocument::SetMajor(int v)
    {
        major_ = v;
    }
    inline int HttpHeaderDocument::GetMinor()
    {
        return minor_;
    }
    inline void HttpHeaderDocument::SetMinor(int v)
    {
        minor_ = v;
    }
    inline std::string const& HttpHeaderDocument::GetField(std::string const& k)
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
    inline void HttpHeaderDocument::SetField(std::string const& k, const char* m)
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
    inline void HttpHeaderDocument::SetField(std::string const& k, std::string const& m)
    {
        return SetField(k, m.c_str());
    }
    /// --------------------------------------------------------

} //namespace rapidhttp 
