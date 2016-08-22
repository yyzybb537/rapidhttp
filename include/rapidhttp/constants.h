#pragma once

#include <string>

namespace rapidhttp {

    // CRLF
    static const std::string c_crlf = "\r\n";

    // HTTP头结束符
    static const std::string c_header_end = "\r\n\r\n";

    // 头部域分隔符
    static const char c_field_split = ':';

} //namespace rapidhttp
