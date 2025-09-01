#ifndef AIVAS_IOT_WIFI_HPP
#define AIVAS_IOT_WIFI_HPP

#include "Component.hpp"
#include "Event.hpp"
#include "SoftTimer.hpp"

class Context;

class WiFi : public Component<Scope::singleton, Context>
{
    static constexpr uint32_t reconnectDelay = 5000;

    struct Helpers;
    friend Helpers;

public:
    WiFi(Context& context, char const* ssid, char const* password);
    WiFi(WiFi const&) = delete;

    [[nodiscard]] char const* ssid() const { return ssid_; }
    [[nodiscard]] char const* password() const { return password_; }

    Event<void()> connectEvent;
    Event<void()> disconnectEvent;

private:
    void connect(bool reconnecting) const;
    void logConnect() const;

    void connected();
    void disconnected();

    char const* ssid_;
    char const* password_;
    std::string const hostname_; // must stay constant
    SoftTimer reconnectTimer_;
    void* handle_{};
};

inline auto wiFi(char const* ssid, char const* password)
{
    return component<WiFi>(ssid, password);
}

#endif
