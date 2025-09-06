#ifndef AIVAS_IOT_AUDIORINGBUFFER_HPP
#define AIVAS_IOT_AUDIORINGBUFFER_HPP

#include <atomic>
#include <memory>
#include <memory_resource>
#include <vector>

#include "Function.hpp"

class AudioBuffer
{
    using Pointer = std::unique_ptr<std::int16_t, Function<void(int16_t const*)>>;

public:
        struct Stats
        {
            std::size_t produced;
            std::size_t consumed;
            std::size_t overruns;
            std::size_t capacity;
            std::size_t size;
        };

    using Visitor = Function<void(int16_t const*)>;

    AudioBuffer(std::size_t capacity, std::size_t frameSize, std::pmr::memory_resource* resource);
    AudioBuffer(const AudioBuffer&) = delete;

    void push(int16_t const* data);
    void drop_except_last(std::size_t count);

    bool pop_nowait(Visitor const& visitor);

    Pointer pop();

    [[nodiscard]] Stats stats() const;

    // Anzahl aktuell belegter Frames (nur für Debug/Monitoring)
    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] std::size_t capacity() const { return capacity_; }
    [[nodiscard]] std::size_t frameSize() const { return frameSize_; }

private:
    [[nodiscard]] int16_t* pointer(std::size_t const seq) { return &frames_[seq % capacity_ * frameSize_]; }

    void pop_done();

    void fast_forward_overrun();

    std::size_t capacity_;
    std::size_t frameSize_;
    std::pmr::vector<std::int16_t> frames_;
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};

    std::atomic<std::size_t> produced_{0};
    std::atomic<std::size_t> consumed_{0};
    std::atomic<std::size_t> overruns_{0};
};

#endif
