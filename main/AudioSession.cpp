#include <bsp/esp-box-3.h>
#include <esp_afe_sr_models.h>
#include <esp_log.h>

#include "AudioSession.hpp"
#include "Display.hpp"
#include "Memory.hpp"

static constexpr auto TAG{"AudioSession"};

static auto initMicrophone()
{
    auto const microphone = bsp_audio_codec_microphone_init();
    assert(microphone != nullptr);

    esp_codec_dev_sample_info_t info = {
        .bits_per_sample = sizeof(AudioSession::sample_type) * 8,
        .channel = 2,
        .channel_mask = 0b11,
        .sample_rate = 16000,
        .mclk_multiple = 0
    };
    ESP_ERROR_CHECK(esp_codec_dev_open(microphone, &info));

    ESP_ERROR_CHECK(esp_codec_dev_set_in_mute(microphone, false));
    ESP_ERROR_CHECK(esp_codec_dev_set_in_gain(microphone, 60.0f));
    return microphone;
}

AudioSession::AfeHandle::AfeHandle()
{
    auto const models = esp_srmodel_init("model");
    assert(models != nullptr);

    auto const config = afe_config_init("MM", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    assert(config != nullptr);
    config->aec_init = false;
    config->ns_init = true;
    config->vad_init = true;
    config->agc_init = false;

    interface = esp_afe_handle_from_config(config);
    assert(interface != nullptr);

    instance = interface->create_from_config(config);
    assert(instance != nullptr);

    enableWakenet();

    afe_config_print(config);
}

void AudioSession::AfeHandle::enableWakenet() const { interface->enable_wakenet(instance); }
void AudioSession::AfeHandle::disableWakenet() const { interface->disable_wakenet(instance); }

void AudioSession::AfeHandle::feed(std::int16_t const* data) const { interface->feed(instance, data); }
afe_fetch_result_t* AudioSession::AfeHandle::fetch() const { return interface->fetch(instance); }
std::size_t AudioSession::AfeHandle::feedChunksize() const { return interface->get_feed_chunksize(instance); }
std::size_t AudioSession::AfeHandle::feedChannelNum() const { return interface->get_feed_channel_num(instance); }
std::size_t AudioSession::AfeHandle::fetchChunksize() const { return interface->get_fetch_chunksize(instance); }
std::size_t AudioSession::AfeHandle::fetchChannelNum() const { return interface->get_fetch_channel_num(instance); }

AudioSession::AudioSession()
    : microphone_{initMicrophone()},
      microphoneBuffer_{afeHandle_.feedChannelNum() * afeHandle_.feedChunksize(), &internal_memory_resource},
      audioBuffer_{16, afeHandle_.fetchChannelNum() * afeHandle_.fetchChunksize(), &psram_memory_resource},
      feedTask_{"audioFeed", {*this, &AudioSession::feedTask}, StackDepth{8192}, Priority{5}, Core{0}},
      detectTask_{"audioDetect", {*this, &AudioSession::detectTask}, StackDepth{8192}, Priority{5}, Core{1}}
{
    Display::get().showText("Warte...");

    ESP_LOGI(TAG, "audio session successfully initialized");
}

AudioSession::~AudioSession()
{
    running_ = false; // TODO: Stop tasks before closing device
    esp_codec_dev_close(microphone_);
}

void AudioSession::feedTask()
{
    while (running_) {
        auto const bytes = microphoneBuffer_.size() * sizeof(microphoneBuffer_[0]);
        ESP_ERROR_CHECK(esp_codec_dev_read(microphone_, microphoneBuffer_.data(), bytes));
        afeHandle_.feed(microphoneBuffer_.data());
    }
}

void AudioSession::detectTask()
{
    enum class Phase { idle, armed, feeding };

    auto phase{Phase::idle};
    uint32_t dropGuard{};
    uint32_t silenceRun{};
    while (running_) {
        auto const result = afeHandle_.fetch();
        if (result == nullptr || result->ret_value == ESP_FAIL) {
            continue;
        }

        assert(result->data_size == audioBuffer_.frameSize() * sizeof(sample_type));
        audioBuffer_.push(result->data);

        static int counter = 0;
        if (++counter % 50 == 0) {
            auto s = audioBuffer_.stats();
            ESP_LOGI("RB", "size=%u/%u produced=%u consumed=%u drops=%u data_size=%d",
                     (unsigned)s.size, (unsigned)s.capacity,
                     (unsigned)s.produced, (unsigned)s.consumed,
                     (unsigned)s.overruns, result->data_size);
        }

        switch (phase) {
            case Phase::idle:
                if (result->wakeup_state == WAKENET_CHANNEL_VERIFIED) {
                    ESP_LOGI(TAG, "wakeword detected, arming voice activity detection");
                    phase = Phase::armed;
                    dropGuard = dropAfterVerifyFrames;
                    silenceRun = 0;
                    afeHandle_.disableWakenet();
                    detectEvent();
                }
                break;

            case Phase::armed:
                if (dropGuard > 0) {
                    --dropGuard;
                } else if (result->vad_state == VAD_SPEECH) {
                    ESP_LOGI(TAG, "voice activity speech detected after verification phase, start feeding");
                    Display::get().showText("Höre zu...");
                    speechEvent();
                    phase = Phase::feeding;
                    silenceRun = 0;
                }
                break;

            case Phase::feeding:
                if (result->vad_state == VAD_SILENCE) {
                    if (++silenceRun >= silenceFramesToIdle) {
                        ESP_LOGI(TAG, "voice activity silence detected, stop feeding");
                        Display::get().showText("Warte...");
                        phase = Phase::idle;
                        silenceEvent();
                        afeHandle_.enableWakenet();
                    }
                } else {
                    silenceRun = 0;
                }
                break;
        }
    }
}
