#pragma once

#include <system_error>
#include <string>
#include <rapidhttp/layer.hpp>

namespace rapidhttp {

enum class eErrorCode
{
    success = 0,
    parse_progress = 1,
    parse_error = 2,
};

class ErrorCategory : public std::error_category
{
public:
    virtual const char* name() const noexcept override 
    {
        return "RapidHttp Error";
    }

    virtual std::string message(int code) const override
    {
        switch (code) {
            case (int)eErrorCode::success:
                return "success";

            case (int)eErrorCode::parse_error:
                return "parse error";

            default:
                return "unkown error";
        }
    }
};

class ParseErrorCategory : public std::error_category
{
public:
    virtual const char* name() const noexcept override 
    {
        return "RapidHttp Parse Error";
    }

    virtual std::string message(int code) const override
    {
        return http_errno_description((http_errno)code);
    }
};

inline std::error_code MakeErrorCode(eErrorCode code)
{
    static ErrorCategory category;
    return std::error_code((int)code, category);
}

inline std::error_code MakeParseErrorCode(int code)
{
    static ParseErrorCategory category;
    return std::error_code(code, category);
}

} //namespace rapidhttp
