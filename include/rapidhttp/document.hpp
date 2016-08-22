#pragma once

namespace rapidhttp {
    HttpDocument::HttpDocument(HttpDocument const& other);
    HttpDocument::HttpDocument(HttpDocument && other);
    HttpDocument& HttpDocument::operator=(HttpDocument const& other);
    HttpDocument& HttpDocument::operator=(HttpDocument && other);

    /// ------------------- parse/generate ---------------------
    /// 流式解析
    // @buf_ref: 外部传入的缓冲区首地址, 再调用Storage前必须保证缓冲区有效且不变.
    // @len: 缓冲区长度
    // @returns：解析完成返回error_code=0, 解析一半返回error_code=1, 解析失败返回其他错误码.
    std::error_code HttpDocument::PartailParse(const char* buf_ref, size_t len)
    {
        const char* pos = buf_ref;
    }

    // 返回解析错误码
    std::error_code HttpDocument::ParseError()
    {
        return ec_;
    }

    // 保存
    void HttpDocument::Storage();

    size_t HttpDocument::ByteSize();

    bool HttpDocument::Write(char *buf_ref, size_t len);
    /// --------------------------------------------------------

    /// ------------------- fields get/set ---------------------
    StringRef HttpDocument::GetMethod();
    
    void HttpDocument::SetMethodRef(const char* buf_ref);

    void HttpDocument::SetMethod(const char* buf);
    /// --------------------------------------------------------

} //namespace rapidhttp 
