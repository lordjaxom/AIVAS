#ifndef AIVAS_IOT_AUDIORINGBUFFER_HPP
#define AIVAS_IOT_AUDIORINGBUFFER_HPP

#include <algorithm>
#include <atomic>
#include <functional>
#include <vector>

#include "Memory.hpp"

class AudioBuffer
{
public:
    struct Stats
    {
        std::size_t produced;
        std::size_t consumed;
        std::size_t overruns;
        std::size_t capacity;
        std::size_t size;
        float drop_rate_ema;
    };

    using Visitor = std::function<void(const uint8_t* data)>;

    AudioBuffer(std::size_t const capacity, std::size_t const itemSize, float const ema_alpha = .05f)
        : capacity_{capacity},
          itemSize_{itemSize},
          ema_alpha_{ema_alpha}
    {
        frames_.resize(capacity * itemSize, {});
    }

    AudioBuffer(const AudioBuffer&) = delete;

    void push(uint8_t const* data)
    {
        auto const head = head_.load(std::memory_order_relaxed);
        std::copy_n(data, itemSize_, pointer(head));

        auto const tail = tail_.load(std::memory_order_relaxed);
        if (auto const distance = head + 1 - tail; distance > capacity_) {
            overruns_.fetch_add(1, std::memory_order_relaxed);
            update_drop_ema(1.f / static_cast<float>(distance));
        } else {
            update_drop_ema(0.f);
        }

        head_.store(head + 1, std::memory_order_release);
        produced_.fetch_add(1, std::memory_order_relaxed);
    }

    void drop_except_last(std::size_t const count)
    {
        fast_forward_overrun();
        auto const head = head_.load(std::memory_order_acquire);
        auto const tail = tail_.load(std::memory_order_relaxed);

        if (auto const new_tail = head - std::min(count, head - tail); new_tail > tail) {
            auto const drops = new_tail - tail;
            overruns_.fetch_add(drops, std::memory_order_relaxed);
            update_drop_ema(static_cast<float>(drops) / (static_cast<float>(drops) + 1.0f));

            tail_.store(new_tail, std::memory_order_release);
        }
    }

    bool pop_nowait(Visitor const& visitor)
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

    [[nodiscard]] Stats stats() const {
        auto const head = head_.load(std::memory_order_acquire);
        auto const tail = tail_.load(std::memory_order_acquire);
        return {
            produced_.load(std::memory_order_relaxed),
            consumed_.load(std::memory_order_relaxed),
            overruns_.load(std::memory_order_relaxed),
            capacity_,
            (std::min(head - tail, capacity_)),
            drop_rate_ema_.load(std::memory_order_relaxed),
        };
    }

    // Anzahl aktuell belegter Frames (nur für Debug/Monitoring)
    [[nodiscard]] std::size_t size() const
    {
        auto const head = head_.load(std::memory_order_acquire);
        auto const tail = tail_.load(std::memory_order_acquire);
        return std::min(head - tail, capacity_);
    }

    [[nodiscard]] std::size_t capacity() const { return capacity_; }
    [[nodiscard]] std::size_t itemSize() const { return itemSize_; }

private:
    [[nodiscard]] uint8_t* pointer(std::size_t const seq) { return &frames_[seq % capacity_ * itemSize_]; }

    void fast_forward_overrun()
    {
        auto const head = head_.load(std::memory_order_acquire);
        if (auto const tail = tail_.load(std::memory_order_relaxed); head - tail > capacity_) {
            tail_.store(head - capacity_, std::memory_order_release);
        }
    }

    void update_drop_ema(float const sample)
    {
        // drop_rate_ema = (1-α) * ema + α * sample
        auto const prev = drop_rate_ema_.load(std::memory_order_relaxed);
        auto const next = prev + ema_alpha_ * (sample - prev);
        drop_rate_ema_.store(next, std::memory_order_relaxed);
    }

    std::size_t capacity_;
    std::size_t itemSize_;
    std::pmr::vector<uint8_t> frames_;
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};

    std::atomic<std::size_t> produced_{0};
    std::atomic<std::size_t> consumed_{0};
    std::atomic<std::size_t> overruns_{0};
    float ema_alpha_;
    std::atomic<float> drop_rate_ema_{0.0f};
};

#endif
