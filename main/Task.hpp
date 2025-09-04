#ifndef AIVAS_IOT_TASK_HPP
#define AIVAS_IOT_TASK_HPP

#include <functional>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Time.hpp"

class Task
{
    static constexpr std::size_t stackSize = 4096;
    static constexpr auto shutdownTimeout = Duration::millis(2000);
    static constexpr auto shutdownPollInterval = Duration::millis(10);

    using Runnable = std::function<void()>;

public:
    Task(char const* name, unsigned priority, int core, Runnable runnable);
    Task(char const* name, Runnable runnable);
    Task(Task const&) = delete;
    ~Task();

private:
    void run() const;

    char const* name_;
    Runnable runnable_;
    TaskHandle_t handle_{};
};

#endif
