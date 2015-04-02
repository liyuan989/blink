#ifndef __BLINK_ATOMIC_H__
#define __BLINK_ATOMIC_H__

#include <blink/Nocopyable.h>

#include <stdint.h>

namespace blink
{

namespace detail
{

template<typename T>
class AtomicInteger : Nocopyable
{
public:
    AtomicInteger()
        : value_(0)
    {
    }

    T get()
    {
        return __sync_val_compare_and_swap(&value_, 0, 0);
    }

    T getAndAdd(T x)
    {
        return __sync_fetch_and_add(&value_, x);
    }

    T getAndSet(T x)
    {
        return __sync_lock_test_and_set(&value_, x);
    }

    T addAndGet(T x)
    {
        return getAndAdd(x) + x;
    }

    T incrementAndGet()
    {
        return addAndGet(1);
    }

    T decrementAndGet()
    {
        return addAndGet(-1);
    }

    void add(T x)
    {
        getAndAdd(x);
    }

    void increment()
    {
        incrementAndGet();
    }

    void decrement()
    {
        decrementAndGet();
    }

private:
    volatile T value_;
};

}  // namespace detail

typedef detail::AtomicInteger<int64_t> AtomicInt64;
typedef detail::AtomicInteger<int32_t> AtomicInt32;
typedef detail::AtomicInteger<int16_t> AtomicInt16;
typedef detail::AtomicInteger<int8_t>  AtomicInt8;

typedef detail::AtomicInteger<uint64_t> AtomicUint64;
typedef detail::AtomicInteger<uint32_t> AtomicUint32;
typedef detail::AtomicInteger<uint16_t> AtomicUint16;
typedef detail::AtomicInteger<uint8_t>  AtomicUint8;

}  // namespace blink

#endif
