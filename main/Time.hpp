#ifndef AIVAS_IOT_TIME_HPP
#define AIVAS_IOT_TIME_HPP

#include <freertos/FreeRTOS.h>

#include <cstdint>

class Duration
{
public:
    static constexpr Duration ticks(uint32_t const ticks) { return pdTICKS_TO_MS(ticks); }
    static constexpr Duration millis(uint32_t const millis) { return millis; }
    static constexpr Duration none() { return 0; }
    static constexpr Duration max() { return UINT32_MAX; }

    [[nodiscard]] constexpr uint32_t millis() const { return millis_; }

    [[nodiscard]] uint32_t ticks() const
    {
        if (millis_ == 0) return 0;
        if (millis_ == UINT32_MAX) return portMAX_DELAY;
        return std::max(pdMS_TO_TICKS(millis_), 1ul); // at least 1
    }

private:
    constexpr Duration(uint32_t const millis) : millis_{millis} {} // NOLINT(*-explicit-constructor)

    uint32_t millis_;
};

#endif
