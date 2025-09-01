#include <utility>

#include "Application.hpp"
#include "SoftTimer.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <portmacro.h>

SoftTimer::SoftTimer(Context& context, char const* name, Handler handler)
    : context_{context},
      name_{name},
      handler_{std::move(handler)},
      handle_{
          xTimerCreate(
              name_, pdMS_TO_TICKS(10), false, this,
              [](auto timer) { static_cast<SoftTimer*>(pvTimerGetTimerID(timer))->timedOut(); }
          )
      }
{
    configASSERT(handle_ != nullptr);
}

SoftTimer::~SoftTimer()
{
    stop();
    xTimerDelete(static_cast<TimerHandle_t>(handle_), portMAX_DELAY);
}

bool SoftTimer::active() const
{
    return xTimerIsTimerActive(static_cast<TimerHandle_t>(handle_));
}

void SoftTimer::start(uint32_t const timeout, bool const repeat) const
{
    auto const handle = static_cast<TimerHandle_t>(handle_);
    vTimerSetReloadMode(handle, repeat);
    xTimerChangePeriod(handle, pdMS_TO_TICKS(timeout), portMAX_DELAY);
    xTimerStart(handle, portMAX_DELAY);
}

void SoftTimer::stop() const
{
    if (active()) {
        xTimerStop(static_cast<TimerHandle_t>(handle_), portMAX_DELAY);
    }
}

void SoftTimer::timedOut() const
{
    context_.dispatch(handler_);
}
