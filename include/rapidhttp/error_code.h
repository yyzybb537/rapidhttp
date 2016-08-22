#pragma once

#include <system_error>

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
    virtual const char* name() const override noexcept
    {
        return "RapidHttp Error";
    }

    virtual string message(int code) const override
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

inline std::error_code MakeErrorCode(eErrorCode code)
{
    static ErrorCategory category;
    return std::error_code((int)code, category);
}

} //namespace rapidhttp
