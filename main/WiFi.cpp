#include <ranges>

#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#include "Application.hpp"
#include "WiFi.hpp"

static constexpr auto TAG = "WiFI";

static String toHostname(std::string_view clientId)
{
    String hostname{clientId};
    std::ranges::transform(
        std::as_const(hostname), hostname.begin(),
        [](auto c) { return std::tolower(c); }
    );
    return str("iot-", clientId);
}

template<std::size_t N>
void copy(std::string_view const src, uint8_t (&dest)[N])
{
    assert(src.length() < N);
    std::ranges::copy(src, reinterpret_cast<char*>(dest));
    dest[src.length()] = 0;
}

struct WiFi::Helpers
{
    static void ipStaGotIpEvent(void* arg, esp_event_base_t, int32_t const, void*)
    {
        static_cast<WiFi*>(arg)->wiFiConnected();
    }

    static void wiFiAnyEvent(void* arg, esp_event_base_t, int32_t const event_id, void*)
    {
        if (event_id == WIFI_EVENT_STA_START) {
            static_cast<WiFi*>(arg)->connectToWiFi(false);
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            static_cast<WiFi*>(arg)->wiFiDisconnected();
        }
    }
};

WiFi::WiFi(std::string_view ssid, std::string_view password)
    : ssid_{ssid},
      password_{password},
      hostname_{toHostname(Application::get().clientId())}
      // reconnectTimer_{context, "wiFiReconnect", [this] { connectToWiFi(true); }}
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

    ESP_ERROR_CHECK(esp_netif_set_hostname(handle_, hostname_.c_str()));

    wifi_init_config_t const initConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&initConfig));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &Helpers::wiFiAnyEvent, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &Helpers::ipStaGotIpEvent, this, nullptr));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t config{};
    copy(ssid_, config.sta.ssid);
    copy(password_, config.sta.password);
    config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));

    // Optional: Stromsparmodus steuern (IDF default ist meist PS MIN)
    // ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    ESP_ERROR_CHECK(esp_wifi_start());
}

void WiFi::connectToWiFi(bool const reconnecting) const
{
    if (reconnecting) logConnect();
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void WiFi::logConnect() const
{
    ESP_LOGI(TAG, "connecting to WiFi at %.*s as %s", ssid_.length(), ssid_.data(), hostname_.c_str());
}

void WiFi::wiFiConnected()
{
    esp_netif_ip_info_t ipInfo;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(handle_, &ipInfo));
    ESP_LOGI(TAG, "connection to WiFi established as " IPSTR, IP2STR(&ipInfo.ip));

    connected_ = true;
    // reconnectTimer_.stop();
    connectEvent();
}

void WiFi::wiFiDisconnected()
{
    ESP_LOGE(TAG, "connection to WiFi lost");

    connected_ = false;
    // reconnectTimer_.start(reconnectDelay);
    disconnectEvent();
}
