#ifndef AIVAS_IOT_WEBSOCKETCLIENT_HPP
#define AIVAS_IOT_WEBSOCKETCLIENT_HPP

#include <span>
#include <string_view>

#include <esp_websocket_client.h>

#include "Function.hpp"
#include "String.hpp"
#include "Time.hpp"

class WebSocket
{
    struct Helpers;
    friend Helpers;

    using Callback = Function<void()>;

public:
    WebSocket(std::string_view host, std::uint16_t port, std::string_view path,
              Callback const& connectCallback = []{}, Callback const& disconnectCallback = []{});
    ~WebSocket();

    [[nodiscard]] bool connected() const { return connected_; }

    void sendBinary(std::span<uint8_t const> payload, Duration timeout = Duration::max()) const;
    void sendText(std::string_view payload, Duration timeout = Duration::max()) const;

private:
    void connectToWs() const;

    void wsConnected();
    void wsDisconnected();

    String const uri_; // must stay constant
    esp_websocket_client_handle_t handle_{};
    bool connected_{};
    Function<void()> connectCallback_;
    Function<void()> disconnectCallback_;
};

#endif
