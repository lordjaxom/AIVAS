#ifndef AIVAS_IOT_LOGGER_HPP
#define AIVAS_IOT_LOGGER_HPP

#include <utility>

#include <esp_log.h>

#include "String.hpp"

class Logger
{
public:
    constexpr explicit Logger(char const* tag) noexcept
        : tag_{tag} {}

    template<typename... Args>
    void info(Args&&... args) const
    {
        ESP_LOGI(tag_, "%s", str(std::forward<Args>(args)...).c_str());
    }

private:
    char const* tag_;
};

#endif
