#ifndef AIVAS_IOT_MQTTCLIENT_HPP
#define AIVAS_IOT_MQTTCLIENT_HPP

#include <functional>
#include <map>
#include <string>
#include <string_view>

#include <mqtt_client.h>

#include "Context.hpp"
#include "Component.hpp"
#include "Event.hpp"
#include "WiFi.hpp"

class MqttClient : public Component<Scope::singleton, Context, WiFi>
{
    using Subscriber = std::function<void(std::string payload)>;

    struct Helpers;
    friend Helpers;

public:
    MqttClient(Context& context, WiFi& wiFi, char const* host, uint16_t port = 1883);
    MqttClient(MqttClient const&) = delete;
    ~MqttClient();

    [[nodiscard]] bool connected() const { return connected_; }

    void publish(std::string const& topic, std::string_view payload, bool retain = false) const;
    void subscribe(std::string topic, Subscriber handler);

private:
    void connectToMqtt();
    void wiFiDisconnected() const;
    void mqttConnected();
    void mqttDisconnected();
    void mqttMessage(char const* topic, char const* payload, size_t length);
    void mqttSubscribe(std::string const& topic) const;

    char const* clientId_;
    std::string const uri_; // must stay constant
    std::string const topic_; // must stay constant
    std::string const willTopic_; // must stay constant
    Subscription wiFiConnected_;
    Subscription wiFiDisconnected_;
    esp_mqtt_client_handle_t handle_{};
    bool connected_{};
    std::multimap<std::string, Subscriber> subscriptions_;
};

inline auto mqttClient(char const* host, uint16_t port = 1883)
{
    return component<MqttClient>(host, port);
}

#endif
