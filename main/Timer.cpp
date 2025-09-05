#include <utility>

#include "Application.hpp"
#include "Timer.hpp"

Timer::Timer(char const* name, Handler handler)
    : name_{name},
      handler_{std::move(handler)},
      handle_{
          xTimerCreate(
              name_, 1, false, this,
              [](auto timer) { static_cast<Timer*>(pvTimerGetTimerID(timer))->timedOut(); }
          )
      }
{
    assert(handle_ != nullptr);
}

Timer::~Timer()
{
    xTimerDelete(handle_, portMAX_DELAY);
}

bool Timer::active() const
{
    return xTimerIsTimerActive(handle_);
}

void Timer::start(Duration const timeout, bool const repeat) const
{
    vTimerSetReloadMode(handle_, repeat);
    xTimerChangePeriod(handle_, timeout.ticks(), portMAX_DELAY);
    xTimerStart(handle_, portMAX_DELAY);
}

void Timer::stop() const
{
    if (active()) {
        xTimerStop(handle_, portMAX_DELAY);
    }
}

void Timer::timedOut() const
{
    Application::get().dispatch(handler_);
}
