#ifndef __BLINK_TYPES_H__
#define __BLINK_TYPES_H__

#ifdef BLINK_STD_STRING

#include <string>

#else // !BLINK_GNU_STRING

#include <ext/vstring.h>
#include <ext/vstring_fwd.h>

#endif

#include <stdint.h>

namespace blink
{

#ifdef BLINK_STD_STRING

using std::string;

#else // !BLINK_GNU_STRING

typedef __gnu_cxx::__sso_string string;

#endif

template<typename To, typename From>
inline To implicit_cast(const From& f)
{
	return f;
}

}  // namespace blink

#endif
