#pragma once

#include <string>
#include <string.h>
#include <assert.h>

namespace rapidhttp {

class StringRef
{
public:
    StringRef()
        : owner_(false), len_(0), str_("")
    {}

    StringRef(const char* str, uint32_t len)
        : owner_(false), len_(len), str_(str)
    {}

    StringRef(StringRef const& other)
    {
        *this = other;
    }

    StringRef& operator=(StringRef const& other)
    {
        if (this == &other) return *this;

        if (owner_)
            free(str_);

        if (other.owner_) {
            str_ = (const char*)malloc(other.len_);
            memcpy(str_, other.str_, other.len_);
        } else
            str_ = other.str_;
        len_ = other.len_;
        owner_ = other.owner_;
        return *this;
    }

    StringRef(StringRef && other)
    {
        *this = std::move(other);
    }

    StringRef& operator=(StringRef && other)
    {
        if (this == &other) return *this;

        if (owner_)
            free(str_);

        str_ = other.str_;
        len_ = other.len_;
        owner_ = other.owner_;

        other.owner_ = false;
        other.len_ = 0;
        other.str_ = "";
        return *this;
    }

    explicit StringRef(std::string const& s)
        : owner_(false), len_(s.size()), str_(s.c_str())
    {}

    ~StringRef()
    {
        if (owner_)
            free(str_);
    }

    const char* c_str() const
    {
        return str_;
    }

    size_t size() const
    {
        return len_;
    }

    std::string ToString() const
    {
        return std::string(str_, len_);
    }

    void SetString(std::string const& s)
    {
        str_ = s.c_str();
        len_ = s.size();
        owner_ = false;
    }

    void SetOwner()
    {
        if (!owner_ && len_) {
            const char* buf = (const char*)malloc(len_);
            memcpy(buf, str_, len_);
            str_ = buf;
            owner_ = true;
        }
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
    bool owner_ : 1
    uint32_t len_ : 31;
    const char* str_;
};

} //namespace rapidhttp
