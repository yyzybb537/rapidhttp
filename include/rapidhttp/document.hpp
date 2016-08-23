#pragma once
#include "document.h"
#include <rapidhttp/util.h>

namespace rapidhttp {
//    HttpDocument::HttpDocument(HttpDocument const& other);
//    HttpDocument::HttpDocument(HttpDocument && other);
//    HttpDocument& HttpDocument::operator=(HttpDocument const& other);
//    HttpDocument& HttpDocument::operator=(HttpDocument && other);

    /// ------------------- parse/generate ---------------------
    /// 流式解析
    // @buf_ref: 外部传入的缓冲区首地址, 再调用Storage前必须保证缓冲区有效且不变.
    // @len: 缓冲区长度
    // @returns：解析完成返回error_code=0, 解析一半返回error_code=1, 解析失败返回其他错误码.
    size_t HttpDocument::PartailParse(std::string const& buf)
    {
        return PartailParse(buf.c_str(), buf.size());
    }
    size_t HttpDocument::PartailParse(const char* buf_ref, size_t len)
    {
        size_t remain_length = 0;
        if (ec_.value() == (int)eErrorCode::parse_error ||
                parse_state_ == eParseState::done) {
            ResetPartailParse();
        } else if (ec_.value() == (int)eErrorCode::parse_progress) {
            remain_length = parse_buffer_.length();
            parse_buffer_.append(buf_ref, len);
            buf_ref = parse_buffer_.c_str();
            len = parse_buffer_.size();
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
            switch (parse_state_) {
                case eParseState::init:
                    {
                        pos = SkipSpaces(pos, last);
                        parse_state_ = (IsRequest()) ? eParseState::method : eParseState::version;
                    }
                    break;

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
                    }
                    break;

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
                        const char* next = FindCRLF(pos, last, ec_);
                        _CHECK_ERRORCODE();
                        _CHECK_PROGRESS(next);

                        if (pos == next) {
                            parse_state_ = eParseState::done;
                            ec_ = std::error_code();
                            parse_buffer_.clear();
                            return next + 2 - buf_ref - remain_length;
                        }

                        std::string key, value;
                        if (!ParseField(pos, next, key, value)) {
                            ec_ = MakeErrorCode(eErrorCode::parse_error);
                            return 0;
                        }

                        pos = next + 2;
                        header_fields_[key].swap(value);
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
    void HttpDocument::ResetPartailParse()
    {
        parse_state_ = eParseState::init;
        ec_ = std::error_code();
        parse_buffer_.clear();
        header_fields_.clear();
    }

    bool HttpDocument::ParseMethod(const char* pos, const char* last)
    {
        parse_buffer_.append(pos, last);
        request_method_.swap(parse_buffer_);
        if (!CheckMethod()) return false;
        parse_buffer_.clear();
        return true;
    }
    bool HttpDocument::ParseUri(const char* pos, const char* last)
    {
        parse_buffer_.append(pos, last);
        request_uri_.swap(parse_buffer_);
        if (!CheckUri()) return false;
        parse_buffer_.clear();
        return true;
    }
    bool HttpDocument::ParseVersion(const char* pos, const char* last)
    {
        if (!parse_buffer_.empty()) {
            parse_buffer_.append(pos, last);
            pos = &parse_buffer_[0];
            last = pos + parse_buffer_.length();
        }

        static const char* sc_http = "HTTP";
        if (*(int*)pos != *(int*)sc_http) return false;
        pos += 4;
        if (*pos++ != '/') return false;
        major_ = *pos++ - '0';
        if (*pos++ != '.') return false;
        minor_ = *pos++ - '0';
        if (!CheckVersion()) return false;
        parse_buffer_.clear();
        return true;
    }
    bool HttpDocument::ParseCode(const char* pos, const char* last)
    {
        if (!parse_buffer_.empty()) {
            parse_buffer_.append(pos, last);
            pos = &parse_buffer_[0];
            last = pos + parse_buffer_.length();
        }

        if (last - pos != 3) return false;
        if (*pos < '1' || *pos > '9') return false;
        response_code_ = (*pos++ - '0') * 100;
        if (*pos < '0' || *pos > '9') return false;
        response_code_ += (*pos++ - '0') * 10;
        if (*pos < '0' || *pos > '9') return false;
        response_code_ += (*pos++ - '0');
        parse_buffer_.clear();
        return true;
    }
    bool HttpDocument::ParseResponseStr(const char* pos, const char* last)
    {
        parse_buffer_.append(pos, last);
        response_str_.swap(parse_buffer_);
        if (!CheckResponseString()) return false;
        parse_buffer_.clear();
        return true;
    }
    bool HttpDocument::ParseField(const char* pos, const char* last,
            std::string & key, std::string & value)
    {
        if (!parse_buffer_.empty()) {
            parse_buffer_.append(pos, last);
            pos = &parse_buffer_[0];
            last = pos + parse_buffer_.length();
        }

        const char* split = pos;
        for (; split < last; ++split) {
            if (*split == ':')
                break;
        }
        if (split == last) return false;
        key.assign(pos, split);
        // TODO: Check Field Key
        ++split;
        if (*split++ != ' ') return false; // Must be split by ":\s"
        value.assign(split, last);
        // TODO: Check Field Value
        return true;
    }

    // 返回解析错误码
    std::error_code HttpDocument::ParseError()
    {
        return ec_;
    }

    // 保存
//    void HttpDocument::Storage()
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
    bool HttpDocument::IsInitialized() const
    {
        if (IsRequest())
            return CheckMethod() && CheckUri() && CheckVersion();
        else
            return CheckVersion() && CheckCode() && CheckResponseString();
    }

    size_t HttpDocument::ByteSize() const
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

    bool HttpDocument::Serialize(char *buf, size_t len)
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

        if (IsRequest()) {
            _WRITE_STRING(request_method_);
            *buf++ = ' ';
            _WRITE_STRING(request_uri_);
            _WRITE_C_STR(" HTTP/", 6);
            *buf++ = major_ + '0';
            *buf++ = '.';
            *buf++ = minor_ + '0';
        } else {
            _WRITE_C_STR(" HTTP/", 6);
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
        return true;
#undef _WRITE_CRLF
#undef _WRITE_C_STR
#undef _WRITE_STRING
    }
    bool HttpDocument::CheckMethod() const
    {
        return !request_method_.empty();
    }
    bool HttpDocument::CheckUri() const
    {
        return !request_uri_.empty() && request_uri_[0] == '/';
    }
    bool HttpDocument::CheckCode() const
    {
        return response_code_ >= 100 && response_code_ < 1000;
    }
    bool HttpDocument::CheckResponseString() const
    {
        return !response_str_.empty();
    }
    bool HttpDocument::CheckVersion() const
    {
        return major_ >= 0 && major_ <= 9 && minor_ >= 0 && minor_ <= 9;
    }
    /// --------------------------------------------------------

    /// ------------------- fields get/set ---------------------
    std::string const& HttpDocument::GetMethod()
    {
        return request_method_;
    }
    
    void HttpDocument::SetMethod(const char* m)
    {
        request_method_ = m;
    }
    void HttpDocument::SetMethod(std::string const& m)
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
    inline std::string const& HttpDocument::GetResponseString()
    {
        return response_str_;
    }
    inline void HttpDocument::SetResponseString(const char* m)
    {
        response_str_ = m;
    }
    inline void HttpDocument::SetResponseString(std::string const& m)
    {
        response_str_ = m;
    }
    inline int HttpDocument::GetCode()
    {
        return response_code_;
    }
    inline void HttpDocument::SetCode(int code)
    {
        response_code_ = code;
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
        auto it = header_fields_.find(k);
        if (header_fields_.end() == it)
            return empty_string;
        else
            return it->second;
    }
    inline void HttpDocument::SetField(std::string const& k, const char* m)
    {
        header_fields_[k] = m;
    }
    inline void HttpDocument::SetField(std::string const& k, std::string const& m)
    {
        header_fields_[k] = m;
    }
    /// --------------------------------------------------------

} //namespace rapidhttp 
