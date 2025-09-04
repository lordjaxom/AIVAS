#ifndef ESPIDF_IOT_ARDUINO_HPP
#define ESPIDF_IOT_ARDUINO_HPP

#include <freertos/FreeRTOS.h>

inline uint64_t millis()
{
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

inline void delay(uint32_t const ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

#endif