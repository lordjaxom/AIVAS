#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>

#include "Application.hpp"
#include "Ringbuffer.hpp"

static void sendComplete(void* handle, void* item)
{
    xRingbufferSendComplete(handle, item);
}

detail::ringbuffer_deleter::ringbuffer_deleter(void* handle, Function const function)
    : handle_{handle},
      function_{function}
{
}

void detail::ringbuffer_deleter::operator()(void* item) const
{
    if (handle_ != nullptr && item != nullptr) {
        function_(handle_, item);
    }
}

RingbufferBase::RingbufferBase(size_t const capacity, size_t const itemSize) noexcept
    : itemSize_{itemSize},
      handle_{xRingbufferCreateNoSplit(itemSize_, capacity)}
{
    configASSERT(handle_ != nullptr);
}

RingbufferBase::~RingbufferBase()
{
    vRingbufferDelete(handle_);
}

RingbufferBase::Pointer RingbufferBase::acquire(uint32_t const timeout) const
{
    void* item;
    if (auto const ticks = timeout != portMAX_DELAY ? pdMS_TO_TICKS(timeout) : portMAX_DELAY; // TODO
        xRingbufferSendAcquire(handle_, &item, itemSize_, ticks)) {
        return {item, detail::ringbuffer_deleter{handle_, &sendComplete}};
    }
    return nullptr;
}

RingbufferBase::Pointer RingbufferBase::receive(uint32_t const timeout) const
{
    size_t itemSize;
    auto const ticks = timeout != portMAX_DELAY ? pdMS_TO_TICKS(timeout) : portMAX_DELAY; // TODO
    void* item = xRingbufferReceive(handle_, &itemSize, ticks);
    return {item, detail::ringbuffer_deleter{handle_, &vRingbufferReturnItem}};
}
