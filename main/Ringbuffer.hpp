#ifndef ESPIDF_IOT_RINGBUFFER_HPP
#define ESPIDF_IOT_RINGBUFFER_HPP

#include <cstddef>
#include <memory>

namespace detail {
    struct ringbuffer_deleter
    {
        using Function = void(*)(void* handle, void* item);
        explicit ringbuffer_deleter(void* handle = nullptr, Function function = [](auto...) {});
        void operator()(void* item) const;

    private:
        void* handle_;
        Function function_;
    };
}

class RingbufferBase
{
public:
    RingbufferBase(const RingbufferBase&) = delete;

protected:
    using Pointer = std::unique_ptr<void, detail::ringbuffer_deleter>;

    RingbufferBase(size_t capacity, size_t itemSize) noexcept;
    ~RingbufferBase();

    [[nodiscard]] Pointer acquire(uint32_t timeout) const;
    [[nodiscard]] Pointer receive(uint32_t timeout) const;

private:
    size_t itemSize_;
    void* handle_{};
};

template<typename T>
class Ringbuffer : public RingbufferBase
{
public:
    using Pointer = std::unique_ptr<T, detail::ringbuffer_deleter>;

    explicit Ringbuffer(size_t const capacity) noexcept
        : RingbufferBase{capacity, sizeof(T)}
    {
    }

    template<typename... Args>
    bool emplace(uint32_t const timeout, Args&&... args) const
    {
        if (auto const itemPtr = acquire(timeout)) {
            new(&*itemPtr) T{std::forward<Args>(args)...};
            return true;
        }
        return false;
    }

    [[nodiscard]] Pointer acquire(uint32_t const timeout) const
    {
        auto itemPtr = RingbufferBase::acquire(timeout);
        return {static_cast<T*>(itemPtr.release()), itemPtr.get_deleter()};
    }

    [[nodiscard]] Pointer receive(uint32_t const timeout) const
    {
        auto itemPtr = RingbufferBase::receive(timeout);
        return {static_cast<T*>(itemPtr.release()), itemPtr.get_deleter()};
    }
};

#endif
