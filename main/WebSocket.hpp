#ifndef AIVAS_IOT_WEBSOCKETCLIENT_HPP
#define AIVAS_IOT_WEBSOCKETCLIENT_HPP

#include <span>
#include <string_view>

#include <esp_websocket_client.h>

#include "Event.hpp"
#include "String.hpp"
#include "Time.hpp"

class WebSocket
{
    struct Helpers;
    friend Helpers;

public:
    WebSocket(char const* host, uint16_t port, char const* path);
    ~WebSocket();

    [[nodiscard]] bool connected() const { return connected_; }

    void sendBinary(std::span<uint8_t const> payload, Duration timeout = Duration::max()) const;
    void sendText(std::string_view payload, Duration timeout = Duration::max()) const;

    Event<void()> connectEvent;
    Event<void()> disconnectEvent;

private:
    void connectToWs() const;

    void wsConnected();
    void wsDisconnected();

    String const uri_; // must stay constant
    esp_websocket_client_handle_t handle_{};
    bool connected_{};
};

#endif