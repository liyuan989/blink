#ifndef __EXAMPLE_MEMCACHED_SERVER_H__
#define __EXAMPLE_MEMCACHED_SERVER_H__

#include "Nocopyable.h"
#include "Types.h"
#include "StringPiece.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace blink
{

class Buffer;

}

class Item;
typedef boost::shared_ptr<Item> ItemPtr;
typedef boost::shared_ptr<const Item> ConstItemPtr;

// Item is immutable once added into hash table
class Item : blink::Nocopyable
{
public:
    enum UpdatePolicy
    {
        kInvalid,
        kSet,
        kAdd,
        kReplaced,
        kAppend,
        kCas,
    };

    Item(blink::StringPiece key_arg,
         uint32_t flags_arg,
         int exptime_arg,
         int value_len,
         uint64_t cas_arg);

    ~Item()
    {
        ::free(data_);
    }

    static ItemPtr makeItem(blink::StringPiece key_arg,
                            uint32_t flags_arg,
                            int exptime_arg,
                            int value_len,
                            uint64_t cas_arg)
    {
        return boost::make_shared<Item>(key_arg, flags_arg, exptime_arg, value_len, cas_arg);
    }

    void append(const char* data, size_t len);
    void output(blink::Buffer* out, bool need_cas = false) const;
    void resetKey(blink::StringPiece k);

    blink::StringPiece key() const
    {
        return blink::StringPiece(data_, key_len_);
    }

    uint32_t flags() const
    {
        return flags_;
    }

    int rel_exptime() const
    {
        return rel_exptime_;
    }

    const char* value() const
    {
        return data_ + key_len_;
    }

    size_t valueLength() const
    {
        return value_len_;
    }

    uint64_t cas() const
    {
        return cas_;
    }

    size_t hash() const
    {
        return hash_;
    }

    void setCas(uint64_t cas)
    {
        cas_ = cas;
    }

    size_t neededBytes() const
    {
        return totalLen() - received_bytes_;
    }

    bool endWithCRLF() const
    {
        return received_bytes_ == totalLen()
            && data_[totalLen() - 2] == '\r'
            && data_[totalLen() - 1] == '\n';
    }

private:
    int totalLen() const
    {
        return key_len_ + value_len_;
    }

    int             key_len_;
    const uint32_t  flags_;
    const int       rel_exptime_;
    const int       value_len_;
    int             received_bytes_;
    uint64_t        cas_;
    size_t          hash_;
    char*           data_;
};

#endif
