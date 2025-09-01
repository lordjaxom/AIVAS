#include <cstring>
#include <algorithm>

#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#include "Context.hpp"
#include "String.hpp"
#include "WiFi.hpp"

static constexpr auto TAG = "WiFI";

static std::string toHostname(std::string clientId)
{
    std::ranges::transform(
        std::as_const(clientId), clientId.begin(),
        [](auto c) { return std::tolower(c); }
    );
    return str("iot-", clientId);
}

struct WiFi::Helpers
{
    static void ipStaGotIpEvent(void* arg, esp_event_base_t, int32_t const, void*)
    {
        static_cast<WiFi*>(arg)->connected();
    }

    static void wiFiAnyEvent(void* arg, esp_event_base_t, int32_t const event_id, void*)
    {
        if (event_id == WIFI_EVENT_STA_START) {
            static_cast<WiFi*>(arg)->connect(false);
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            static_cast<WiFi*>(arg)->disconnected();
        }
    }
};

WiFi::WiFi(Context& context, char const* ssid, char const* password)
    : ssid_{ssid},
      password_{password},
      hostname_{toHostname(context.clientId())},
      reconnectTimer_{context, "wiFiReconnect", [this] { connect(true); }}
{
    logConnect();

    if (auto const err = nvs_flash_init();
        err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    handle_ = esp_netif_create_default_wifi_sta();
    configASSERT(handle_ != nullptr);

    ESP_ERROR_CHECK(esp_netif_set_hostname(static_cast<esp_netif_t*>(handle_), hostname_.c_str()));

    wifi_init_config_t const initConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&initConfig));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &Helpers::wiFiAnyEvent, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &Helpers::ipStaGotIpEvent, this, nullptr));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t config{};
    std::strncpy(reinterpret_cast<char*>(config.sta.ssid), ssid_, sizeof(config.sta.ssid));
    std::strncpy(reinterpret_cast<char*>(config.sta.password), password_, sizeof(config.sta.password));
    config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));

    // Optional: Stromsparmodus steuern (IDF default ist meist PS MIN)
    // ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    ESP_ERROR_CHECK(esp_wifi_start());
}
void WiFi::connect(bool const reconnecting) const
{
    if (reconnecting) {
        logConnect();
    }
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void WiFi::logConnect() const
{
    ESP_LOGI(TAG, "connecting to WiFi at %s as %s", ssid_, hostname_.c_str());
}

void WiFi::connected()
{
    esp_netif_ip_info_t ipInfo;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(static_cast<esp_netif_t*>(handle_), &ipInfo));
    ESP_LOGI(TAG, "connection to WiFi established as " IPSTR, IP2STR(&ipInfo.ip));

    reconnectTimer_.stop();

    connectEvent();
}

void WiFi::disconnected()
{
    ESP_LOGE(TAG, "connection to WiFi lost");

    reconnectTimer_.start(reconnectDelay);

    disconnectEvent();
}
