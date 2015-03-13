#ifndef __BLINK_CONDITION_H__
#define __BLINK_CONDITION_H__

#include <blink/MutexLock.h>

namespace blink
{

class Condition : Nocopyable
{
public:
	explicit Condition(MutexLock& mutex);
	~Condition();

	void wait();
	bool timedWait(int seconds);  // if timeout return true, false otherwise
	void wakeup();
	void wakeupAll();

private:
	MutexLock&      mutex_;
	pthread_cond_t  cond_;
};

}  // namespace blink

#endif
