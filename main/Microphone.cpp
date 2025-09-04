#include <bsp/esp-box-3.h>
#include <esp_codec_dev.h>
#include <esp_log.h>

#include "Microphone.hpp"

static constexpr auto TAG{"Microphone"};

Microphone::Microphone()
    : handle_{bsp_audio_codec_microphone_init()}
{
    configASSERT(handle_ != nullptr);

    esp_codec_dev_sample_info_t info = {
        .bits_per_sample = 16,
        .channel = 2,
        .channel_mask = 0b11,
        .sample_rate = 16000,
        .mclk_multiple = 0
    };
    ESP_ERROR_CHECK(esp_codec_dev_open(handle_, &info));

    ESP_ERROR_CHECK(esp_codec_dev_set_in_mute(handle_, false));
    ESP_ERROR_CHECK(esp_codec_dev_set_in_gain(handle_, 30.0f));

    ESP_LOGI(TAG, "microphone successfully initialized");
}

Microphone::~Microphone()
{
    esp_codec_dev_close(handle_);
}

void Microphone::read(void* buffer, std::size_t const size) const
{
    ESP_ERROR_CHECK(esp_codec_dev_read(handle_, buffer, size));
}
