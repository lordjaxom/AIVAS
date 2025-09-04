#include <algorithm>

#include <esp_log.h>

#include "MqttClient.hpp"

static constexpr auto TAG{"MqttClient"};

static String toTopic(String clientId)
{
    std::ranges::replace(clientId, '-', '/');
    return clientId;
}

struct MqttClient::Helpers
{
    static void mqttClientAnyEvent(void* arg, esp_event_base_t, int32_t const event_id, void* event_data)
    {
        auto* client = static_cast<MqttClient*>(arg);
        auto const* data = static_cast<esp_mqtt_event_handle_t>(event_data);
        switch (event_id) {
            case MQTT_EVENT_CONNECTED:
                client->mqttConnected();
                break;
            case MQTT_EVENT_DISCONNECTED:
                client->mqttDisconnected();
                break;
            case MQTT_EVENT_DATA:
                client->mqttMessage(data->topic, data->data, data->data_len);
                break;
            default:
                break;
        }
    }
};

MqttClient::MqttClient(Context& context, WiFi& wiFi, char const* host, uint16_t const port)
    : clientId_{context.clientId()},
      uri_{str("mqtt://", host, ":", port)},
      topic_{toTopic(clientId_)},
      willTopic_{str("tele/", topic_, "/LWT")},
      wiFiConnected_{wiFi.connectEvent.subscribe([this] { connectToMqtt(); })},
      wiFiDisconnected_{wiFi.disconnectEvent.subscribe([this] { wiFiDisconnected(); })}
{
    esp_mqtt_client_config_t config = {};
    config.broker.address.uri = uri_.c_str();
    config.credentials.client_id = context.clientId();
    config.session.last_will.topic = willTopic_.c_str();
    config.session.last_will.msg = "Offline";
    config.session.last_will.retain = 1;

    // Auto-Reconnect ist default true; Keepalive default 120s
    // ggf. cfg.session.keepalive = 60; cfg.session.disable_clean_session = false; etc.

    handle_ = esp_mqtt_client_init(&config);
    configASSERT(handle_);

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(handle_, MQTT_EVENT_ANY, &Helpers::mqttClientAnyEvent, this));

    if (wiFi.connected()) connectToMqtt();
}

MqttClient::~MqttClient()
{
    esp_mqtt_client_stop(handle_);
    esp_mqtt_client_destroy(handle_);
}

void MqttClient::publish(String const& topic, std::string_view const payload, bool const retain) const
{
    if (!connected_) return;
    esp_mqtt_client_publish(handle_, topic.c_str(), payload.data(), payload.size(), 1, retain);
}

void MqttClient::subscribe(String topic, Subscriber handler)
{
    if (auto const it = subscriptions_.emplace(std::move(topic), std::move(handler));
        connected_ && subscriptions_.count(it->first) == 1) {
        mqttSubscribe(it->first);
    }
}

void MqttClient::connectToMqtt() const
{
    ESP_LOGI(TAG, "connexting to MQTT broker at %s as %s", uri_.c_str(), clientId_);
    ESP_ERROR_CHECK(esp_mqtt_client_start(handle_));
}

void MqttClient::wiFiDisconnected() const
{
    ESP_ERROR_CHECK(esp_mqtt_client_stop(handle_));
}

void MqttClient::mqttConnected()
{
    ESP_LOGI(TAG, "connection to MQTT broker established");

    connected_ = true;
    publish(willTopic_, "Online", true);
    for (auto it = subscriptions_.cbegin(); it != subscriptions_.cend(); it = subscriptions_.upper_bound(it->first)) {
        mqttSubscribe(it->first);
    }
}

void MqttClient::mqttDisconnected()
{
    ESP_LOGI(TAG, "connection to MQTT broker lost");
    connected_ = false;
}

void MqttClient::mqttMessage(char const* topic, char const* payload, size_t const length)
{
    String message{payload, length};

    ESP_LOGI(TAG, "received '%s' at topic %s", message.c_str(), topic);

    auto [begin, end] = subscriptions_.equal_range(topic);
    std::for_each(begin, end, [&](auto const& pair) { pair.second(message); });
}

void MqttClient::mqttSubscribe(String const& topic) const
{
    ESP_LOGI(TAG, "subscribing to topic %s", topic.c_str());
    esp_mqtt_client_subscribe(handle_, topic.c_str(), 1);
}
