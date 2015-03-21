#ifndef __EXAMPLE_BLINK_HASH_H__
#define __EXAMPLE_BLINK_HASH_H__

#include <blink/Types.h>

namespace boost
{

std::size_t hash_value(const blink::string& x);

}  // namespace boost

#include <boost/unordered_map.hpp>

namespace boost
{

inline std::size_t hash_value(const blink::string& x)
{
    return hash_range(x.begin(), x.end());
}

}  // namespace boost

typedef boost::unordered_map<blink::string, int64_t> WordCountMap;

#endif
