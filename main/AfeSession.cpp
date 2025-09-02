#include <esp_afe_sr_models.h>

#include "AfeSession.hpp"

#include "Logger.hpp"

static Logger logger("AfeSession");

AfeSession::AfeSession(Context& context, Microphone& microphone, Display& display)
    : context_{context},
      microphone_{microphone},
      display_{display}
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

    feedTask_.emplace("afeFeed", 5, 0, [this] { feedTask(); });
    detectTask_.emplace("afeDetect", 5, 1, [this] { detectTask(); });

    display_.postText("Listening...");

    logger.info("audio front end session successfully initialized");
}

AfeSession::~AfeSession()
{
    interface_->destroy(instance_);
}

afe_fetch_result_t* AfeSession::fetch() const
{
    return interface_->fetch(instance_);
}

void AfeSession::feedTask()
{
    auto const bytes = buffer_.size() * sizeof(buffer_[0]);
    microphone_.read(buffer_.data(), bytes);
    interface_->feed(instance_, buffer_.data());
}

void AfeSession::detectTask()
{
    auto const result = interface_->fetch(instance_);
    if (result == nullptr || result->ret_value == ESP_FAIL) {
        return;
    }

    if (result->wakeup_state == WAKENET_DETECTED) {
        logger.info("wakeword detected");
    } else if (result->wakeup_state == WAKENET_CHANNEL_VERIFIED) {
        logger.info("wakenet channel verified");
        detected_ = true;
        frameKeep_ = 0;
        interface_->disable_wakenet(instance_);
        display_.postText("Processing...");
    }

    if (detected_) {
        if (state_ != result->vad_state) {
            state_ = result->vad_state;
            frameKeep_ = 0;
        } else {
            ++frameKeep_;
        }

        if (frameKeep_ == 100 && result->vad_state == VAD_SILENCE) {
            logger.info("silence detected");
            interface_->enable_wakenet(instance_);
            detected_ = false;
            display_.postText("Listening...");
        }
    }
}
