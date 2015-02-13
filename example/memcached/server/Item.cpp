#include "Item.h"
#include "Buffer.h"
#include "LogStream.h"

#include <boost/unordered_map.hpp>

#include <string.h>
#include <stdio.h>

using namespace blink;

Item::Item(StringPiece key_arg,
           uint32_t flags_arg,
           int exptime_arg,
           int value_len,
           uint64_t cas_arg)
    : key_len_(key_arg.size()),
      flags_(flags_arg),
      rel_exptime_(exptime_arg),
      value_len_(value_len),
      received_bytes_(0),
      cas_(cas_arg),
      hash_(boost::hash_range(key_arg.begin(), key_arg.end())),
      data_(static_cast<char*>(::malloc(totalLen())))
{
    assert(value_len_ >= 2);
    assert(received_bytes_ < totalLen());
    append(key_arg.data(), key_len_);
}

void Item::append(const char* data, size_t len)
{
    assert(len <= neededBytes());
    memcpy(data_ + received_bytes_, data, len);
    received_bytes_ += static_cast<int>(len);
    assert(received_bytes_ <= totalLen());
}

void Item::output(Buffer* out, bool need_cas) const
{
    out->append("VALUE ");
    out->append(data_, key_len_);
    LogStream buf;
    buf << ' ' << flags_ << ' ' << value_len_ - 2;
    if (need_cas)
    {
        buf << ' ' << cas_;
    }
    buf << "\r\n";
    out->append(buf.buffer().data(), buf.buffer().usedSize());
    out->append(value(), value_len_);
}

void Item::resetKey(StringPiece k)
{
    assert(k.size() <= 250);
    key_len_ = k.size();
    received_bytes_ = 0;
    append(k.data(), k.size());
    hash_ = boost::hash_range(k.begin(), k.end());
}
