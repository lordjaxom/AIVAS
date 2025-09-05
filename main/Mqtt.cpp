#include <algorithm>

#include <esp_log.h>

#include "Application.hpp"
#include "Mqtt.hpp"
#include "WiFi.hpp"

static constexpr auto TAG{"Mqtt"};

static String toBaseTopic(std::string_view clientId)
{
    String baseTopic{clientId};
    std::ranges::replace(baseTopic, '-', '/');
    return baseTopic;
}

struct Mqtt::Helpers
{
    static void mqttClientAnyEvent(void* arg, esp_event_base_t, int32_t const event_id, void* event_data)
    {
        auto* client = static_cast<Mqtt*>(arg);
        auto const* data = static_cast<esp_mqtt_event_handle_t>(event_data);
        switch (event_id) {
            case MQTT_EVENT_CONNECTED:
                client->mqttConnected();
                break;
            case MQTT_EVENT_DISCONNECTED:
                client->mqttDisconnected();
                break;
            case MQTT_EVENT_DATA:
                client->mqttMessage(
                    {data->topic, static_cast<std::size_t>(data->topic_len)},
                    {data->data, static_cast<std::size_t>(data->data_len)}
                );
                break;
            default:
                break;
        }
    }
};

Mqtt::Mqtt(std::string_view host, uint16_t const port)
    : uri_{str("mqtt://", host, ":", port)},
      baseTopic_{toBaseTopic(Application::get().clientId())},
      willTopic_{str("tele/", baseTopic_, "/LWT")},
      wiFiConnected_{WiFi::get().connectEvent.connect({*this, &Mqtt::connectToMqtt})},
      wiFiDisconnected_{WiFi::get().disconnectEvent.connect({*this, &Mqtt::wiFiDisconnected})}
{
    String copyOfClientId{Application::get().clientId()};
    esp_mqtt_client_config_t config = {};
    config.broker.address.uri = uri_.c_str();
    config.credentials.client_id = copyOfClientId.c_str();
    config.session.last_will.topic = willTopic_.c_str();
    config.session.last_will.msg = "Offline";
    config.session.last_will.retain = 1;

    // Auto-Reconnect ist default true; Keepalive default 120s
    // ggf. cfg.session.keepalive = 60; cfg.session.disable_clean_session = false; etc.

    handle_ = esp_mqtt_client_init(&config);
    assert(handle_ != nullptr);

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(handle_, MQTT_EVENT_ANY, &Helpers::mqttClientAnyEvent, this));

    if (WiFi::get().connected()) connectToMqtt();
}

Mqtt::~Mqtt()
{
    esp_mqtt_client_stop(handle_);
    esp_mqtt_client_destroy(handle_);
}

void Mqtt::publish(String const& topic, std::string_view const payload, bool const retain) const
{
    publish(topic.c_str(), payload, retain);
}

void Mqtt::publish(char const* topic, std::string_view const payload, bool const retain) const
{
    if (!connected_) return;
    esp_mqtt_client_publish(handle_, topic, payload.data(), payload.size(), 1, retain);
}

void Mqtt::subscribe(String topic, Subscriber handler)
{
    if (auto const it = subscriptions_.emplace(std::move(topic), std::move(handler));
        connected_ && subscriptions_.count(it->first) == 1) {
        mqttSubscribe(it->first);
    }
}

void Mqtt::connectToMqtt()
{
    ESP_LOGI(TAG, "connecting to MQTT broker at %s as %.*s", uri_.c_str(), Application::get().clientId().length(),
             Application::get().clientId().data());

    ESP_ERROR_CHECK(esp_mqtt_client_start(handle_));
    started_ = true;
}

void Mqtt::wiFiDisconnected()
{
    if (started_) {
        ESP_ERROR_CHECK(esp_mqtt_client_stop(handle_));
        started_ = false;
    }
}

void Mqtt::mqttConnected()
{
    ESP_LOGI(TAG, "connection to MQTT broker established");

    connected_ = true;
    publish(willTopic_, "Online", true);
    for (auto it = subscriptions_.cbegin(); it != subscriptions_.cend(); it = subscriptions_.upper_bound(it->first)) {
        mqttSubscribe(it->first);
    }
}

void Mqtt::mqttDisconnected()
{
    ESP_LOGI(TAG, "connection to MQTT broker lost");
    connected_ = false;
}

void Mqtt::mqttMessage(std::string_view const topic, std::string_view const payload)
{
    ESP_LOGI(TAG, "received '%.*s' at topic %.*s", payload.length(), payload.data(), topic.length(), topic.data());

    auto [begin, end] = subscriptions_.equal_range(topic);
    std::for_each(begin, end, [&](auto const& pair) { pair.second(payload); });
}

void Mqtt::mqttSubscribe(String const& topic) const
{
    ESP_LOGI(TAG, "subscribing to topic %s", topic.c_str());
    esp_mqtt_client_subscribe(handle_, topic.c_str(), 1);
}
