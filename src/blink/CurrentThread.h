#ifndef __BLINK_CURRENTTHREAD_H__
#define __BLINK_CURRENTTHREAD_H__

#include <stdint.h>

namespace blink
{

extern __thread int t_cache_tid;
extern __thread char t_tid_string[32];
extern __thread int t_tid_string_length;
extern __thread const char* t_thread_name;

void cacheTid();

inline int tid()
{
	if (__builtin_expect(t_cache_tid == 0, 0))
	{
		cacheTid();
	}
	return t_cache_tid;
}

inline const char* tidString()
{
	return t_tid_string;
}

inline int tidStringLength()
{
	return t_tid_string_length;
}

inline const char* threadName()
{
	return t_thread_name;
}

bool isMainThread();

void sleepMicroseconds(int64_t microseconds);  // implement by nanosleep.

}  // namespace blink

#endif
