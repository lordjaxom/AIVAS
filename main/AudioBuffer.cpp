#include <algorithm>

#include "AudioBuffer.hpp"
#include "Memory.hpp"

AudioBuffer::AudioBuffer(std::size_t const capacity, std::size_t const frameSize, std::pmr::memory_resource* resource)
    : capacity_{capacity},
      frameSize_{frameSize},
      frames_{capacity * frameSize, resource}
{
}

void AudioBuffer::push(int16_t const* data)
{
    auto const head = head_.load(std::memory_order_relaxed);
    std::copy_n(data, frameSize_, pointer(head));

    auto const tail = tail_.load(std::memory_order_relaxed);
    if (auto const distance = head + 1 - tail; distance > capacity_) {
        overruns_.fetch_add(1, std::memory_order_relaxed);
    }

    head_.store(head + 1, std::memory_order_release);
    produced_.fetch_add(1, std::memory_order_relaxed);
}

void AudioBuffer::drop_except_last(std::size_t const count)
{
    fast_forward_overrun();
    auto const head = head_.load(std::memory_order_acquire);
    auto const tail = tail_.load(std::memory_order_relaxed);

    if (auto const new_tail = head - std::min(count, head - tail); new_tail > tail) {
        auto const drops = new_tail - tail;
        overruns_.fetch_add(drops, std::memory_order_relaxed);
        tail_.store(new_tail, std::memory_order_release);
    }
}

bool AudioBuffer::pop_nowait(Visitor const& visitor)
{
    fast_forward_overrun();

    auto const head = head_.load(std::memory_order_acquire);
    auto const tail = tail_.load(std::memory_order_relaxed);
    if (tail == head) return false;

    visitor(pointer(tail));
    tail_.store(tail + 1, std::memory_order_release);

    consumed_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

AudioBuffer::Stats AudioBuffer::stats() const
{
    auto const head = head_.load(std::memory_order_acquire);
    auto const tail = tail_.load(std::memory_order_acquire);
    return {
        produced_.load(std::memory_order_relaxed),
        consumed_.load(std::memory_order_relaxed),
        overruns_.load(std::memory_order_relaxed),
        capacity_,
        (std::min(head - tail, capacity_)),
    };
}

std::size_t AudioBuffer::size() const
{
    auto const head = head_.load(std::memory_order_acquire);
    auto const tail = tail_.load(std::memory_order_acquire);
    return std::min(head - tail, capacity_);
}

void AudioBuffer::fast_forward_overrun()
{
    auto const head = head_.load(std::memory_order_acquire);
    if (auto const tail = tail_.load(std::memory_order_relaxed); head - tail > capacity_) {
        tail_.store(head - capacity_, std::memory_order_release);
    }
}
