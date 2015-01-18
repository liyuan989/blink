#include "Atomic.h"

#include <assert.h>
#include <stdio.h>

using namespace blink;

int main(int argc, char const *argv[])
{
	AtomicInt64 atomic_int64;
	assert(atomic_int64.get() == 0);
	assert(atomic_int64.getAndAdd(10) == 0);
	assert(atomic_int64.getAndSet(20) == 10);
	assert(atomic_int64.addAndGet(20) == 40);
	assert(atomic_int64.incrementAndGet() == 41);
	assert(atomic_int64.decrementAndGet() == 40);
	return 0;
}
