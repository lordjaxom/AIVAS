#ifndef AIVAS_IOT_MQTTCLIENT_HPP
#define AIVAS_IOT_MQTTCLIENT_HPP

#include <functional>
#include <map>
#include <string_view>

#include <mqtt_client.h>

#include "Context.hpp"
#include "Component.hpp"
#include "Event.hpp"
#include "String.hpp"
#include "WiFi.hpp"

class MqttClient : public Component<Scope::singleton, Context, WiFi>
{
    using Subscriber = std::function<void(String payload)>;

    struct Helpers;
    friend Helpers;

public:
    MqttClient(Context& context, WiFi& wiFi, char const* host, uint16_t port = 1883);
    MqttClient(MqttClient const&) = delete;
    ~MqttClient();

    [[nodiscard]] bool connected() const { return connected_; }

    void publish(String const& topic, std::string_view payload, bool retain = false) const;
    void subscribe(String topic, Subscriber handler);

private:
    void connectToMqtt() const;
    void wiFiDisconnected() const;
    void mqttConnected();
    void mqttDisconnected();
    void mqttMessage(char const* topic, char const* payload, size_t length);
    void mqttSubscribe(String const& topic) const;

    char const* clientId_;
    String const uri_; // must stay constant
    String const topic_; // must stay constant
    String const willTopic_; // must stay constant
    Subscription wiFiConnected_;
    Subscription wiFiDisconnected_;
    esp_mqtt_client_handle_t handle_{};
    bool connected_{};
    std::multimap<String, Subscriber> subscriptions_;
};

inline auto mqttClient(char const* host, uint16_t port = 1883)
{
    return component<MqttClient>(host, port);
}

#endif
