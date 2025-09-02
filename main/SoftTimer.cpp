#include <utility>

#include "Application.hpp"
#include "SoftTimer.hpp"

#include <portmacro.h>

SoftTimer::SoftTimer(Context& context, char const* name, Handler handler)
    : context_{context},
      name_{name},
      handler_{std::move(handler)},
      handle_{
          xTimerCreate(
              name_, 1, false, this,
              [](auto timer) { static_cast<SoftTimer*>(pvTimerGetTimerID(timer))->timedOut(); }
          )
      }
{
    configASSERT(handle_ != nullptr);
}

SoftTimer::~SoftTimer()
{
    stop();
    xTimerDelete(handle_, portMAX_DELAY);
}

bool SoftTimer::active() const
{
    return xTimerIsTimerActive(handle_);
}

void SoftTimer::start(Duration const timeout, bool const repeat) const
{
    auto const handle = static_cast<TimerHandle_t>(handle_);
    vTimerSetReloadMode(handle, repeat);
    xTimerChangePeriod(handle, timeout.ticks(), portMAX_DELAY);
    xTimerStart(handle, portMAX_DELAY);
}

void SoftTimer::stop() const
{
    if (active()) {
        xTimerStop(handle_, portMAX_DELAY);
    }
}

void SoftTimer::timedOut() const
{
    context_.dispatch(handler_);
}
