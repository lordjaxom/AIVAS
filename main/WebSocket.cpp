#include "String.hpp"
#include "WebSocket.hpp"

#include "esp_log.h"

static constexpr auto TAG{"WebSocket"};

struct WebSocket::Helpers
{
    static void wsClientAnyEvent(void* arg, esp_event_base_t, int32_t const event_id, void* event_data)
    {
        auto* ws = static_cast<WebSocket*>(arg);
        auto* data = static_cast<esp_websocket_event_data_t*>(event_data);
        switch (event_id) {
            case WEBSOCKET_EVENT_CONNECTED:
                ws->wsConnected();
                break;
            case WEBSOCKET_EVENT_DISCONNECTED:
                ws->wsDisconnected();
                break;
            case WEBSOCKET_EVENT_DATA:
                // ws->wsMessage(data->data_ptr, data->data_len, data->op_code);
                break;
            default:
                break;
        }
    }
};

WebSocket::WebSocket(std::string_view const host, std::uint16_t const port, std::string_view const path,
                     Callback const& connectCallback, Callback const& disconnectCallback)
    : uri_{str("ws://", host, ":", port, path)},
      connectCallback_{connectCallback},
      disconnectCallback_{disconnectCallback}
{
    esp_websocket_client_config_t config = {};
    config.uri = uri_.c_str();

    handle_ = esp_websocket_client_init(&config);
    assert(handle_ != nullptr);

    ESP_ERROR_CHECK(esp_websocket_register_events(handle_, WEBSOCKET_EVENT_ANY, &Helpers::wsClientAnyEvent, this));

    connectToWs();
}

WebSocket::~WebSocket()
{
    esp_websocket_client_stop(handle_);
    esp_websocket_client_destroy(handle_);
}

void WebSocket::sendBinary(std::span<uint8_t const> const payload, Duration const timeout) const
{
    if (!connected_) return;
    esp_websocket_client_send_bin(
        handle_,
        reinterpret_cast<char const*>(payload.data()),
        static_cast<int>(payload.size()),
        timeout.ticks()
    );
}

void WebSocket::sendText(std::string_view const payload, Duration const timeout) const
{
    if (!connected_) return;
    esp_websocket_client_send_text(handle_, payload.data(), static_cast<int>(payload.size()), timeout.ticks());
}

void WebSocket::connectToWs() const
{
    ESP_LOGI(TAG, "connecting to WebSocket at %s", uri_.c_str());
    ESP_ERROR_CHECK(esp_websocket_client_start(handle_));
}

void WebSocket::wsConnected()
{
    ESP_LOGI(TAG, "connection to WebSocket established");

    connected_ = true;
    connectCallback_();
}

void WebSocket::wsDisconnected()
{
    ESP_LOGI(TAG, "connection to WebSocket lost");

    connected_ = false;
    disconnectCallback_();
}
