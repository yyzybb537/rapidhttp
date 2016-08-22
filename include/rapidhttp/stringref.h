#pragma once

#include <string>
#include <string.h>

namespace rapidhttp {

class StringRef
{
public:
    StringRef()
        : pos_(0), len_(0), str_("")
    {}

    StringRef(const char* str, size_t pos, size_t len)
        : pos_(pos), len_(len), str_(str)
    {}
    
    StringRef(const char* str, size_t len)
        : pos_(0), len_(len), str_(str)
    {}

    explicit StringRef(std::string const& s)
        : pos_(0), len_(s.size()), str_(s.c_str())
    {}

    void OnCopy(const char* str)
    {
        str_ = str;
    }

    const char* c_str() const
    {
        return str_ + pos_;
    }

    size_t size() const
    {
        return len_;
    }

    std::string ToString() const
    {
        return std::string(str_ + pos_, len_);
    }

    void SetString(std::string const& s)
    {
        str_ = s.c_str();
        pos_ = 0;
        len_ = s.size();
    }

    /// ------------- string equal-compare operator ---------------
public:
    friend bool operator==(StringRef const& lhs, StringRef const& rhs)
    {
        if (lhs.size() != rhs.size()) return false;
        if (lhs.c_str() == rhs.c_str()) return true;
        return memcmp(lhs.c_str(), rhs.c_str(), lhs.size()) == 0;
    }
    friend bool operator!=(StringRef const& lhs, StringRef const& rhs)
    {
        return !(lhs == rhs);
    }
    friend bool operator==(StringRef const& lhs, std::string const& rhs)
    {
        if (lhs.size() != rhs.size()) return false;
        return memcmp(lhs.c_str(), rhs.c_str(), lhs.size()) == 0;
    }
    friend bool operator!=(StringRef const& lhs, std::string const& rhs)
    {
        return !(lhs == rhs);
    }
    friend bool operator==(std::string const& lhs, StringRef const& rhs)
    {
        return rhs == lhs;
    }
    friend bool operator!=(std::string const& lhs, StringRef const& rhs)
    {
        return !(lhs == rhs);
    }
    friend bool operator==(StringRef const& lhs, const char* rhs)
    {
        assert(rhs);
        if (*lhs.c_str() != *rhs) return false;
        size_t len = strlen(rhs);
        if (lhs.size() != len) return false;
        return memcmp(lhs.c_str(), rhs, lhs.size()) == 0;
    }
    friend bool operator!=(StringRef const& lhs, const char* rhs)
    {
        assert(rhs);
        return !(lhs == rhs);
    }
    friend bool operator==(const char* lhs, StringRef const& rhs)
    {
        assert(lhs);
        return rhs == lhs;
    }
    friend bool operator!=(const char* lhs, StringRef const& rhs)
    {
        assert(lhs);
        return !(lhs == rhs);
    }
    /// -----------------------------------------------------

private:
    size_t pos_;
    size_t len_;
    const char* str_;
};

} //namespace rapidhttp
