#ifndef ESPIDF_IOT_SOFTTIMER_HPP
#define ESPIDF_IOT_SOFTTIMER_HPP

#include <freertos/timers.h>

#include "Function.hpp"
#include "Time.hpp"

class Timer
{
    using Handler = Function<void()>;

public:
    Timer(char const* name, Handler const& handler);
    Timer(Timer const&) = delete;
    ~Timer();

    [[nodiscard]] bool active() const;

    void start(Duration timeout, bool repeat = false) const;
    void stop() const;

private:
    void timedOut() const;

    char const* name_;
    Handler handler_;
    TimerHandle_t handle_;
};

#endif
