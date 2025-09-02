#ifndef AIVAS_IOT_TIME_HPP
#define AIVAS_IOT_TIME_HPP

#include <cstdint>

class Duration
{
public:
    static constexpr Duration millis(uint32_t const millis) { return Duration{millis}; }
    static constexpr Duration none() { return Duration{0}; }
    static constexpr Duration max() { return Duration{UINT32_MAX}; }
    static Duration ticks(uint32_t ticks);

    [[nodiscard]] constexpr uint32_t millis() const { return millis_; }
    [[nodiscard]] uint32_t ticks() const;

private:
    constexpr explicit Duration(uint32_t const millis) : millis_{millis} {}

    uint32_t millis_;
};

#endif
