#pragma once

#include <string>
#include <string.h>
#include <stdlib.h>
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
        if (other.owner_ && other.len_) {
            char* buf = (char*)malloc(other.len_);
            memcpy(buf, other.str_, other.len_);
            str_ = buf;
        } else
            str_ = other.str_;
        len_ = other.len_;
        owner_ = other.owner_;
    }

    StringRef& operator=(StringRef const& other)
    {
        if (this == &other) return *this;

        if (owner_)
            free((void*)str_);

        if (other.owner_ && other.len_) {
            char* buf = (char*)malloc(other.len_);
            memcpy(buf, other.str_, other.len_);
            str_ = buf;
        } else
            str_ = other.str_;
        len_ = other.len_;
        owner_ = other.owner_;
        return *this;
    }

    StringRef(StringRef && other)
    {
        str_ = other.str_;
        len_ = other.len_;
        owner_ = other.owner_;

        other.owner_ = false;
        other.len_ = 0;
        other.str_ = "";
    }

    StringRef& operator=(StringRef && other)
    {
        if (this == &other) return *this;

        if (owner_)
            free((void*)str_);

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
            free((void*)str_);
    }

    const char* c_str() const
    {
        return str_;
    }

    size_t size() const
    {
        return len_;
    }

    bool empty() const
    {
        return !size();
    }

    void clear()
    {
        if (owner_)
            free((void*)str_);

        str_ = "";
        len_ = 0;
        owner_ = false;
    }

    operator std::string() const
    {
        return std::string(str_, len_);
    }

    void SetString(std::string const& s)
    {
        if (owner_)
            free((void*)str_);

        str_ = s.c_str();
        len_ = s.size();
        owner_ = false;
    }

    void SetOwner()
    {
        if (!owner_ && len_) {
            char* buf = (char*)malloc(len_);
            memcpy(buf, str_, len_);
            str_ = buf;
            owner_ = true;
        }
    }

    void append(const char* first, size_t length)
    {
        append(first, first + length);
    }

    void append(const char* first, const char* last)
    {
        if (first >= last) return ;

        if (!len_) {
            str_ = first;
            len_ = last - first;
        } else if (!owner_ && str_ + len_ == first) {
            len_ += last - first;
        } else {
            size_t new_len = len_ + (last - first);
            char* buf = nullptr;
            if (owner_) {
                buf = (char*)realloc((void*)str_, new_len);
            } else {
                buf = (char*)malloc(new_len);
                memcpy(buf, str_, len_);
            }

            memcpy(buf + len_, first, last - first);
            str_ = buf;
            len_ = new_len;
            owner_ = true;
        }
    }

    /// ------------- string assign operator ---------------
public:
    StringRef& operator=(const char* cstr)
    {
        clear();
        str_ = cstr;
        len_ = strlen(str_);
        return *this;
    }

    StringRef& operator=(std::string const& s)
    {
        SetString(s);
        return *this;
    }

    char const& operator[](int index) const
    {
        assert(index >= 0 && index < len_);
        return str_[index];
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
    bool owner_ : 1;
    uint32_t len_ : 31;
    const char* str_;
};

} //namespace rapidhttp
