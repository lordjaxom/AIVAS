#ifndef ESPIDF_IOT_RINGBUFFER_HPP
#define ESPIDF_IOT_RINGBUFFER_HPP

#include <memory>

#include "Time.hpp"

namespace detail {
    struct ringbuffer_deleter
    {
        using ReturnFn = void(*)(void* handle, void* item);
        using DestroyFn = void(*)(void* item);
        explicit ringbuffer_deleter(void* handle = nullptr, ReturnFn returnFn = nullptr);
        ringbuffer_deleter& destroy(DestroyFn destroyFn);
        void operator()(void* item) const;

    private:
        void* handle_;
        ReturnFn returnFn_;
        DestroyFn destroyFn_{};
    };
}

template<typename T>
class Ringbuffer;

template<>
class Ringbuffer<void>
{
    using Pointer = std::unique_ptr<void, detail::ringbuffer_deleter>;

public:
    Ringbuffer(size_t capacity, size_t itemSize);
    ~Ringbuffer();
    Ringbuffer(Ringbuffer const&) = delete;

    [[nodiscard]] std::size_t capacity() const { return capacity_; }
    [[nodiscard]] std::size_t itemSize() const { return itemSize_; }

    [[nodiscard]] Pointer acquire(Duration timeout = Duration::max()) const;
    [[nodiscard]] Pointer receive(Duration timeout = Duration::max()) const;

private:
    std::size_t capacity_;
    std::size_t itemSize_;
    void* handle_{};
};

template<typename T>
class Ringbuffer : Ringbuffer<void>
{
    using Base = Ringbuffer<void>;
    using Pointer = std::unique_ptr<T, detail::ringbuffer_deleter>;

public:
    explicit Ringbuffer(size_t const capacity) noexcept
        : Base{capacity, sizeof(T)}
    {
    }

    template<typename... Args>
    Pointer acquire(Duration const timeout, Args&&... args) const
    {
        if (auto itemPtr = Base::acquire(timeout)) {
            new(static_cast<T*>(itemPtr.get())) T{std::forward<Args>(args)...};
            return {static_cast<T*>(itemPtr.release()), itemPtr.get_deleter()};
        }
        return nullptr;
    }

    template<typename... Args>
    Pointer acquire(Args&&... args) const
    {
        return acquire(Duration::max(), std::forward<Args>(args)...);
    }

    [[nodiscard]] Pointer receive(Duration const timeout = Duration::max()) const
    {
        auto itemPtr = Base::receive(timeout);
        auto destroyFn = [](auto item) { std::destroy_at(static_cast<T*>(item)); };
        return {static_cast<T*>(itemPtr.release()), itemPtr.get_deleter().destroy(destroyFn)};
    }
};

#endif
