// Reference from PCRE pcre_stringpiece.h
//
// Copyright (c) 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Sanjay Ghemawat
//
// A string like object that points into another piece of memory.
// Useful for providing an interface that allows clients to easily
// pass in either a "const char*" or a "string".
//
// Arghh!  I wish C++ literals were automatically of type "string".

#ifndef __BLINK_STRINGPIECE_H__
#define __BLINK_STRINGPIECE_H__

#include <blink/Types.h>

#ifndef BLINK_STD_STRING
#include <string>
#endif

#include <iosfwd>  // for ostream forward-declaration
#include <string.h>

namespace blink
{

// For passing C-style string argument to a function.
class StringArg
{
public:
    StringArg(const char* str)
        : str_(str)
    {
    }

    StringArg(const string& str)
        : str_(str.c_str())
    {
    }

#ifndef BLINK_STD_STRING
    StringArg(const std::string& str)
        : str_(str.c_str())
    {
    }
#endif

    const char* c_str() const
    {
        return str_;
    }

private:
    const char* str_;
};

class StringPiece
{
public:
    // We provide non-explicit singleton constructor so users can pass
    // in a "const char" or a "string" wherever a "StringArg" is expected.
    StringPiece()
        : ptr_(NULL), length_(0)
    {
    }

    StringPiece(const char* str)
        : ptr_(str),
          length_(static_cast<int>(strlen(ptr_)))
    {
    }

    StringPiece(const unsigned char* str)
        : ptr_(reinterpret_cast<const char*>(str)),
          length_(static_cast<int>(strlen(ptr_)))
    {
    }

    StringPiece(const string& str)
        : ptr_(str.data()),
          length_(static_cast<int>(str.size()))
    {
    }

#ifndef BLINK_STD_STRING
    StringPiece(const std::string& str)
        : ptr_(str.data()),
          length_(static_cast<int>(str.size()))
        {
        }
#endif

    StringPiece(const char* offset, int len)
        : ptr_(offset), length_(len)
    {
    }

    // data() may return a pointer to a buffer with embedded NULLs, and the
    // returned buffer may or may not be null terminated. Therefore it is
    // typically a mistake to pass data() to a routine that expect a NULL
    // terminated string. Use "asString().c_str()" if you really need to do
    // this. Or better yet, change your routine so it does not rely on NULL
    // termination.
    const char* data() const
    {
        return ptr_;
    }

    int size() const
    {
        return length_;
    }

    bool empty() const
    {
        return length_ == 0;
    }

    const char* begin() const
    {
        return ptr_;
    }

    const char* end() const
    {
        return ptr_ + length_;
    }

    void clear()
    {
        ptr_ = NULL;
        length_ = 0;
    }

    void set(const char* buffer, int len)
    {
        ptr_ = buffer;
        length_ = len;
    }

    void set(const void* buffer, int len)
    {
        ptr_ = reinterpret_cast<const char*>(buffer);
        length_ = len;
    }

    void set(const char* str)
    {
        ptr_ = str;
        length_ = static_cast<int>(strlen(str));
    }

    void removePrefix(int n)
    {
        ptr_ += n;
        length_ -= n;
    }

    void removeSuffix(int n)
    {
        length_ -= n;
    }

    char operator[](int i) const
    {
        return ptr_[i];
    }

    bool operator==(const StringPiece& x) const
    {
        return ((length_ == x.length_) &&
                (memcmp(ptr_, x.ptr_, length_) == 0));
    }

    bool operator!=(const StringPiece& x) const
    {
        return !(*this == x);
    }

#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp)                                 \
    bool operator cmp(const StringPiece& x) const                                 \
    {                                                                             \
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);  \
        return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));           \
    }

    STRINGPIECE_BINARY_PREDICATE(<, <);
    STRINGPIECE_BINARY_PREDICATE(<=, <);
    STRINGPIECE_BINARY_PREDICATE(>=, >);
    STRINGPIECE_BINARY_PREDICATE(>, >);
#undef STRINGPIECE_BINARY_PREDICATE

    int compare(const StringPiece& x) const
    {
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
        if (r == 0)
        {
            if (length_ < x.length_)
            {
                return -1;
            }
            else if (length_ > x.length_)
            {
                return 1;
            }
        }
        return r;
    }

    string asString() const
    {
        return string(data(), size());
    }

    void copyToString(string* target) const
    {
        target->assign(ptr_, length_);
    }

#ifndef BLINK_STD_STRING
    void copyToString(std::string* target) const
    {
        target->assign(ptr_, length_);
    }
#endif

    // Dose "this" start with "x"
    bool startWith(const StringPiece& x) const
    {
        return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, length_) == 0));
    }

private:
    const char*  ptr_;
    int          length_;
};

}  // namespace blink

// ---------------------------------------------------------------
// Functions used to create STL, containers that use StringPiece
// Remember that a StringPiece's lifetime had better be less than
// that of the underlying string or char*. If it is not , then you
// cannot safely store a StringPiece into an STL container
// ---------------------------------------------------------------

#ifdef HAVE_TYPE_TRAITS

// This make vector<StringPiece> really fast for some STL implementions
template<>
struct __type_trait<blink::StringPiece>
{
    typedef __true_type  has_trivial_default_constructor;
    typedef __true_type  has_trivial_copy_constructor;
    typedef __true_type  has_trivial_assignment_operator;
    typedef __true_type  has_trivial_destructor;
    typedef __true_type  is_POD_type;
};

#endif

// allow StringPiece to be logged
std::ostream& operator<<(std::ostream& o, const blink::StringPiece& piece);

#endif
