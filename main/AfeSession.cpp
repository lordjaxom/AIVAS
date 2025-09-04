#include <algorithm>

#include <esp_afe_sr_models.h>
#include <esp_log.h>

#include "AfeSession.hpp"

static constexpr auto TAG{"AfeSession"};

AfeSession::AfeSession(Context& context, Microphone& microphone)
    : context_{context},
      microphone_{microphone}
{
    auto const models = esp_srmodel_init("model");
    configASSERT(models != nullptr);

    auto const config = afe_config_init("MM", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    configASSERT(config != nullptr);
    config->aec_init = false;
    config->ns_init = true;
    config->vad_init = true;
    config->agc_init = true;

    interface_ = esp_afe_handle_from_config(config);
    configASSERT(interface_ != nullptr);

    instance_ = interface_->create_from_config(config);
    configASSERT(instance_ != nullptr);

    auto const enabled = interface_->enable_wakenet(instance_);
    configASSERT(enabled == 1);

    afe_config_print(config);

    auto const feed_channels = interface_->get_feed_channel_num(instance_);
    auto const feed_chunksize = interface_->get_feed_chunksize(instance_);
    buffer_.resize(feed_channels * feed_chunksize);

    auto const fetch_channels = interface_->get_fetch_channel_num(instance_);
    auto const fetch_chunksize = interface_->get_fetch_chunksize(instance_);
    audioBuffer_.emplace(16, fetch_channels * fetch_chunksize * sizeof(int16_t));

    feedTask_.emplace("afeFeed", 5, 0, [this] { feedTask(); });
    detectTask_.emplace("afeDetect", 5, 1, [this] { detectTask(); });

    ESP_LOGI(TAG, "audio front end session successfully initialized");
}

AfeSession::~AfeSession()
{
    running_ = false;
    detectTask_.reset();
    feedTask_.reset();
    interface_->destroy(instance_);
}

void AfeSession::feedTask()
{
    while (running_) {
        auto const bytes = buffer_.size() * sizeof(buffer_[0]);
        microphone_.read(buffer_.data(), bytes);
        interface_->feed(instance_, buffer_.data());
    }
}

void AfeSession::detectTask()
{
    auto phase{Phase::idle};
    uint32_t dropGuard{};
    uint32_t silenceRun{};
    while (running_) {
        auto const result = interface_->fetch(instance_);
        if (result == nullptr || result->ret_value == ESP_FAIL) {
            return;
        }

        configASSERT(result->data_size == audioBuffer_->itemSize());
        audioBuffer_->push(reinterpret_cast<uint8_t const*>(result->data));

        static int counter = 0;
        if (++counter % 50 == 0) {
            auto s = audioBuffer_->stats();
            ESP_LOGI("RB", "size=%u/%u produced=%u consumed=%u drops=%u dropEMA=%.3f",
                     (unsigned)s.size, (unsigned)s.capacity,
                     (unsigned)s.produced, (unsigned)s.consumed,
                     (unsigned)s.overruns, s.drop_rate_ema);
        }

        switch (phase) {
            case Phase::idle:
                if (result->wakeup_state == WAKENET_CHANNEL_VERIFIED) {
                    ESP_LOGI(TAG, "wakeword detected - arming VAD");
                    phase = Phase::armed;
                    dropGuard = dropAfterVerifyFrames;
                    silenceRun = 0;
                    interface_->disable_wakenet(instance_);
                    detectEvent();
                }
                break;

            case Phase::armed:
                if (dropGuard > 0) {
                    --dropGuard;
                } else if (result->vad_state == VAD_SPEECH) {
                    ESP_LOGI(TAG, "vad speech detected after verification phase - start feeding");
                    startEvent();
                    // startEventWithPreroll ???
                    phase = Phase::feeding;
                    silenceRun = 0;
                }
                break;

            case Phase::feeding:
                if (result->vad_state == VAD_SILENCE) {
                    ++silenceRun;
                    if (++silenceRun >= silenceFramesToIdle) {
                        ESP_LOGI(TAG, "vad silence detected - stopping feeding");
                        phase = Phase::idle;
                        interface_->enable_wakenet(instance_);
                        silenceEvent();
                    }
                } else {
                    silenceRun = 0;
                }
                break;
        }
    }
}
