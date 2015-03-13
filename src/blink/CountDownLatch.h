#ifndef __BLINK_COUNTDOWNLATCH_H__
#define __BLINK_COUNTDOWNLATCH_H__

#include <blink/Nocopyable.h>
#include <blink/MutexLock.h>
#include <blink/Condition.h>

namespace blink
{

class CountDownLatch : Nocopyable
{
public:
	explicit CountDownLatch(int count);

	void wait();
	void countDown();
	int getCount() const;

private:
	mutable MutexLock  mutex_;
	Condition          cond_;
	int                count_;
};

}  // namespace blink

#endif
