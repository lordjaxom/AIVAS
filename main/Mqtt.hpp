#ifndef AIVAS_IOT_MQTT_HPP
#define AIVAS_IOT_MQTT_HPP

#include <functional>
#include <map>
#include <string_view>

#include <mqtt_client.h>

#include "Event.hpp"
#include "Function.hpp"
#include "Singleton.hpp"
#include "String.hpp"

class Mqtt : public Singleton<Mqtt>
{
    using Subscriber = Function<void(std::string_view payload)>;

public:
    explicit Mqtt(std::string_view host, uint16_t port = 1883);
    Mqtt(Mqtt const&) = delete;
    ~Mqtt();

    [[nodiscard]] String const& baseTopic() const { return baseTopic_; }

    [[nodiscard]] bool connected() const { return connected_; }

    void publish(String const& topic, std::string_view payload, bool retain = false) const;
    void publish(char const* topic, std::string_view payload, bool retain = false) const;
    void subscribe(String topic, Subscriber handler);

private:
    static void mqttClientEventHandler(void* arg, esp_event_base_t, int32_t event_id, void* event_data);

    void connectToMqtt();
    void wiFiDisconnected();
    void mqttConnected();
    void mqttDisconnected();
    void mqttMessage(std::string_view topic, std::string_view payload);
    void mqttSubscribe(String const& topic) const;

    String const uri_;
    String const baseTopic_;
    String const willTopic_;
    Subscription wiFiConnected_;
    Subscription wiFiDisconnected_;
    esp_mqtt_client_handle_t handle_{};
    bool started_{};
    bool connected_{};
    std::pmr::multimap<String, Subscriber, std::less<>> subscriptions_;
};

#endif
