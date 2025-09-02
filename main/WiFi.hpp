#ifndef AIVAS_IOT_WIFI_HPP
#define AIVAS_IOT_WIFI_HPP

#include <esp_netif_types.h>

#include "Component.hpp"
#include "Event.hpp"
#include "SoftTimer.hpp"

class Context;

class WiFi : public Component<Scope::singleton, Context>
{
    static constexpr Duration reconnectDelay{Duration::millis(5000)};

    struct Helpers;
    friend Helpers;

public:
    WiFi(Context& context, char const* ssid, char const* password);
    WiFi(WiFi const&) = delete;

    [[nodiscard]] char const* ssid() const { return ssid_; }
    [[nodiscard]] char const* password() const { return password_; }

    [[nodiscard]] bool connected() const { return connected_; }

    Event<void()> connectEvent;
    Event<void()> disconnectEvent;

private:
    void connectToWiFi(bool reconnecting) const;
    void logConnect() const;

    void wiFiConnected();
    void wiFiDisconnected();

    char const* ssid_;
    char const* password_;
    std::string const hostname_; // must stay constant
    SoftTimer reconnectTimer_;
    esp_netif_t* handle_{};
    bool connected_{};
};

inline auto wiFi(char const* ssid, char const* password)
{
    return component<WiFi>(ssid, password);
}

#endif
