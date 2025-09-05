#ifndef AIVAS_IOT_WIFI_HPP
#define AIVAS_IOT_WIFI_HPP

#include <string_view>

#include <esp_netif_types.h>

#include "Event.hpp"
#include "Singleton.hpp"
// #include "SoftTimer.hpp"
#include "String.hpp"
#include "Time.hpp"

class Context;

class WiFi : public Singleton<WiFi>
{
    static constexpr Duration reconnectDelay{Duration::millis(5000)};

    struct Helpers;
    friend Helpers;

public:
    WiFi(std::string_view ssid, std::string_view password);

    [[nodiscard]] std::string_view ssid() const { return ssid_; }
    [[nodiscard]] std::string_view password() const { return password_; }

    [[nodiscard]] bool connected() const { return connected_; }

    SubscribeEvent<void()> connectEvent;
    SubscribeEvent<void()> disconnectEvent;

private:
    void connectToWiFi(bool reconnecting) const;
    void logConnect() const;

    void wiFiConnected();
    void wiFiDisconnected();

    std::string_view ssid_;
    std::string_view password_;
    String const hostname_;
    // SoftTimer reconnectTimer_;
    esp_netif_t* handle_{};
    bool connected_{};
};

#endif
