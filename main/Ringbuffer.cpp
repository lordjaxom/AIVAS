#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>

#include "Application.hpp"
#include "Ringbuffer.hpp"

detail::ringbuffer_deleter::ringbuffer_deleter(void* handle, ReturnFn const returnFn)
    : handle_(handle),
      returnFn_(returnFn)
{
}
detail::ringbuffer_deleter& detail::ringbuffer_deleter::destroy(DestroyFn const destroyFn)
{
    destroyFn_ = destroyFn;
    return *this;
}

void detail::ringbuffer_deleter::operator()(void* item) const
{
    if (handle_ != nullptr) {
        if (destroyFn_ != nullptr) destroyFn_(item);
        returnFn_(handle_, item);
    }
}

Ringbuffer<void>::Ringbuffer(size_t const capacity, size_t const itemSize)
    : itemSize_{itemSize},
      handle_{xRingbufferCreateNoSplit(itemSize_, capacity)}
{
    configASSERT(handle_ != nullptr);
}

Ringbuffer<void>::~Ringbuffer()
{
    vRingbufferDelete(handle_);
}

Ringbuffer<void>::Pointer Ringbuffer<void>::acquire(Duration const timeout) const
{
    void* item;
    if (xRingbufferSendAcquire(handle_, &item, itemSize_, timeout.ticks())) {
        return {item, detail::ringbuffer_deleter{handle_, [](auto... args) { xRingbufferSendComplete(args...); }}};
    }
    return nullptr;
}

Ringbuffer<void>::Pointer Ringbuffer<void>::receive(Duration const timeout) const
{
    size_t itemSize;
    if (void* item = xRingbufferReceive(handle_, &itemSize, timeout.ticks()); item != nullptr) {
        return {item, detail::ringbuffer_deleter{handle_, &vRingbufferReturnItem}};
    }
    return nullptr;
}
