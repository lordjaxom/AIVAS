#ifndef AIVAS_IOT_WEBSOCKETCLIENT_HPP
#define AIVAS_IOT_WEBSOCKETCLIENT_HPP

#include <string_view>
#include <string>

#include <esp_websocket_client.h>

#include "Event.hpp"
#include "Time.hpp"

class WebSocket
{
    struct Helpers;
    friend Helpers;

public:
    WebSocket(char const* host, uint16_t port, char const* path);
    ~WebSocket();

    [[nodiscard]] bool connected() const { return connected_; }

    void sendBinary(char const* payload, size_t length, Duration timeout = Duration::max()) const;
    void sendText(std::string_view payload, Duration timeout = Duration::max()) const;

    Event<void()> connectEvent;
    Event<void()> disconnectEvent;

private:
    void connectToWs() const;

    void wsConnected();
    void wsDisconnected();

    std::string const uri_; // must stay constant
    esp_websocket_client_handle_t handle_{};
    bool connected_{};
};

#endif