#include <freertos/ringbuf.h>

#include "Queue.hpp"

detail::queue_deleter::queue_deleter(void* handle, return_fn_type const return_fn)
    : handle_{handle},
      return_fn_{return_fn}
{
}

detail::queue_deleter& detail::queue_deleter::destroy_with(destroy_fn_type const destroy_fn)
{
    destroy_fn_ = destroy_fn;
    return *this;
}

void detail::queue_deleter::operator()(void* item) const
{
    if (handle_ != nullptr) {
        if (destroy_fn_ != nullptr) destroy_fn_(item);
        return_fn_(handle_, item);
    }
}

Queue<void>::Queue(size_t const capacity, size_t const itemSize)
    : capacity_{capacity},
      itemSize_{itemSize},
      handle_{xRingbufferCreateNoSplit(itemSize_, capacity)}
{
    configASSERT(handle_ != nullptr);
}

Queue<void>::~Queue()
{
    vRingbufferDelete(handle_);
}

Queue<void>::Pointer Queue<void>::acquire(Duration const timeout) const
{
    void* item;
    if (xRingbufferSendAcquire(handle_, &item, itemSize_, timeout.ticks())) {
        return {item, detail::queue_deleter{handle_, [](auto... args) { xRingbufferSendComplete(args...); }}};
    }
    return nullptr;
}

Queue<void>::Pointer Queue<void>::receive(Duration const timeout) const
{
    size_t itemSize;
    if (void* item = xRingbufferReceive(handle_, &itemSize, timeout.ticks()); item != nullptr) {
        return {item, detail::queue_deleter{handle_, &vRingbufferReturnItem}};
    }
    return nullptr;
}
