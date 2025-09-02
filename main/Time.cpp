#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

#include "Time.hpp"

Duration Duration::ticks(uint32_t const ticks)
{
    return Duration{pdTICKS_TO_MS(ticks)};
}

uint32_t Duration::ticks() const
{
    return millis_ != UINT32_MAX ? pdMS_TO_TICKS(millis_) : portMAX_DELAY;
}
