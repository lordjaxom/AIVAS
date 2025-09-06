#ifndef AIVAS_IOT_TASK_HPP
#define AIVAS_IOT_TASK_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Function.hpp"
#include "Time.hpp"

namespace detail {
    template<typename T, T Default>
    struct arg
    {
        using value_type = T;
        static constexpr T default_ = Default;
        T value;
    };

    template<typename T, typename... Args>
    constexpr auto get_arg(Args... args)
    {
        static_assert((0 + ... + std::is_same_v<Args, T>) <= 1, "argument specified more than once");

        auto result{T::default_};
        ((std::is_same_v<Args, T> ? (result = args.value, true) : false) || ...);
        return result;
    }
}

struct StackDepth : detail::arg<uint32_t, 4096> {};
struct Priority : detail::arg<unsigned, 2> {};
struct Core : detail::arg<int, tskNO_AFFINITY> {};

class Task
{
    static constexpr auto shutdownTimeout = Duration::millis(2000);
    static constexpr auto shutdownPollInterval = Duration::millis(10);

    using Runnable = Function<void()>;

public:
    template<typename... Args>
    Task(char const* name, Runnable runnable, Args... args)
        : Task{
            name,
            detail::get_arg<StackDepth>(args...),
            detail::get_arg<Priority>(args...),
            detail::get_arg<Core>(args...),
            std::move(runnable)
        }
    {
    }

    Task(Task const&) = delete;
    ~Task();

private:
    Task(char const* name, uint32_t stackDepth, unsigned priority, int core, Runnable const& runnable);

    void run() const;

    char const* name_;
    Runnable runnable_;
    TaskHandle_t handle_{};
};

#endif
