#ifndef AIVAS_IOT_RINGBUFFER_HPP
#define AIVAS_IOT_RINGBUFFER_HPP

#include <memory>

#include "Time.hpp"

namespace detail {
    struct queue_deleter
    {
        using return_fn_type = void(*)(void* handle, void* item);
        using destroy_fn_type = void(*)(void* item);

        explicit queue_deleter(void* handle = nullptr, return_fn_type return_fn = nullptr);
        queue_deleter& destroy_with(destroy_fn_type destroy_fn);
        void operator()(void* item) const;

    private:
        void* handle_;
        return_fn_type return_fn_;
        destroy_fn_type destroy_fn_{};
    };
}

template<typename T>
class Queue;

template<>
class Queue<void>
{
    using Pointer = std::unique_ptr<void, detail::queue_deleter>;

public:
    Queue(size_t capacity, size_t itemSize);
    ~Queue();
    Queue(Queue const&) = delete;

    [[nodiscard]] std::size_t capacity() const { return capacity_; }
    [[nodiscard]] std::size_t itemSize() const { return itemSize_; }

    [[nodiscard]] Pointer acquire(Duration timeout = Duration::max()) const;
    [[nodiscard]] Pointer receive(Duration timeout = Duration::max()) const;
    bool sendFromISR(const void* item) const;

private:
    std::size_t capacity_;
    std::size_t itemSize_;
    void* handle_{};
};

template<typename T>
class Queue : Queue<void>
{
    using Base = Queue<void>;
    using Pointer = std::unique_ptr<T, detail::queue_deleter>;

public:
    explicit Queue(size_t const capacity) noexcept
        : Base{capacity, sizeof(T)}
    {
    }

    template<typename... Args>
    void emplace(Duration const timeout, Args&&... args) const
    {
        (void) acquire(timeout, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void emplace(Args&&... args) const
    {
        (void) acquire(std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[nodiscard]] Pointer acquire(Duration const timeout, Args&&... args) const
    {
        if (auto itemPtr = Base::acquire(timeout)) {
            new(static_cast<T*>(itemPtr.get())) T{std::forward<Args>(args)...};
            return {static_cast<T*>(itemPtr.release()), itemPtr.get_deleter()};
        }
        return nullptr;
    }

    template<typename... Args>
    [[nodiscard]] Pointer acquire(Args&&... args) const
    {
        return acquire(Duration::max(), std::forward<Args>(args)...);
    }

    [[nodiscard]] Pointer receive(Duration const timeout = Duration::max()) const
    {
        auto itemPtr = Base::receive(timeout);
        auto destroy = [](auto item) { std::destroy_at(static_cast<T*>(item)); };
        return {static_cast<T*>(itemPtr.release()), itemPtr.get_deleter().destroy_with(destroy)};
    }

    bool sendFromISR(T const& item) const
    {
        static_assert(std::is_trivially_copyable_v<T>);
        return Base::sendFromISR(&item);
    }
};

#endif
