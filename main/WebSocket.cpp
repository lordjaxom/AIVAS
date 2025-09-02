#include "Logger.hpp"
#include "String.hpp"
#include "WebSocket.hpp"

static constexpr Logger logger{"WebSocket"};

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

WebSocket::WebSocket(char const* host, uint16_t port, char const* path)
    : uri_(str("ws://", host, ":", port, path))
{
    esp_websocket_client_config_t config = {};
    config.uri = uri_.c_str();

    handle_ = esp_websocket_client_init(&config);
    configASSERT(handle_ != nullptr);

    ESP_ERROR_CHECK(esp_websocket_register_events(handle_, WEBSOCKET_EVENT_ANY, &Helpers::wsClientAnyEvent, this));

    connectToWs();
}

WebSocket::~WebSocket()
{
    esp_websocket_client_stop(handle_);
    esp_websocket_client_destroy(handle_);
}

void WebSocket::sendBinary(char const* payload, size_t const length, Duration const timeout) const
{
    if (!connected_) return;
    esp_websocket_client_send_bin(handle_, payload, length, timeout.ticks());
}

void WebSocket::sendText(std::string_view const payload, Duration const timeout) const
{
    if (!connected_) return;
    esp_websocket_client_send_text(handle_, payload.data(), payload.size(), timeout.ticks());
}

void WebSocket::connectToWs() const
{
    logger.info("connecting to WebSocket at ", uri_);
    ESP_ERROR_CHECK(esp_websocket_client_start(handle_));
}

void WebSocket::wsConnected()
{
    logger.info("connection to WebSocket established");

    connected_ = true;
    connectEvent();
}

void WebSocket::wsDisconnected()
{
    logger.info("connection to WebSocket lost");

    connected_ = false;
    disconnectEvent();
}
