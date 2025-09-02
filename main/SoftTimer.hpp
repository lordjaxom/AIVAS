#ifndef ESPIDF_IOT_SOFTTIMER_HPP
#define ESPIDF_IOT_SOFTTIMER_HPP

#include <functional>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include "Time.hpp"

class Context;

class SoftTimer
{
    using Handler = std::function<void()>;

public:
    SoftTimer(Context& context, char const* name, Handler handler);
    SoftTimer(SoftTimer const&) = delete;
    ~SoftTimer();

    [[nodiscard]] bool active() const;

    void start(Duration timeout, bool repeat = false) const;
    void stop() const;

private:
    void timedOut() const;

    Context& context_;
    char const* name_;
    std::function<void()> handler_;
    TimerHandle_t handle_;
};

#endif
