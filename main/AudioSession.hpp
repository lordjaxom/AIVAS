#ifndef AIVAS_IOT_AUDIOSESSION_HPP
#define AIVAS_IOT_AUDIOSESSION_HPP

#include <cstdint>
#include <vector>

#include <esp_afe_sr_iface.h>
#include <esp_codec_dev.h>

#include "AudioBuffer.hpp"
#include "Event.hpp"
#include "Singleton.hpp"
#include "Task.hpp"

class AudioSession : public Singleton<AudioSession>
{
    static constexpr std::uint32_t dropAfterVerifyFrames = 3;
    static constexpr std::uint32_t silenceFramesToIdle = 40;

    struct AfeHandle
    {
        AfeHandle();

        void enableWakenet() const;
        void disableWakenet() const;
        void feed(std::int16_t const* data) const;
        [[nodiscard]] afe_fetch_result_t* fetch() const;

        [[nodiscard]] std::size_t feedChannelNum() const;
        [[nodiscard]] std::size_t feedChunksize() const;
        [[nodiscard]] std::size_t fetchChannelNum() const;
        [[nodiscard]] std::size_t fetchChunksize() const;

    private:
        esp_afe_sr_iface_t* interface{};
        esp_afe_sr_data_t* instance{};
    };

public:
    using sample_type = std::int16_t;

    AudioSession();
    AudioSession(AudioSession const&) = delete;
    ~AudioSession();

    [[nodiscard]] AudioBuffer& audioBuffer() { return audioBuffer_; }

    SubscribeEvent<void()> detectEvent;
    SubscribeEvent<void()> speechEvent;
    SubscribeEvent<void()> silenceEvent;

private:
    void feedTask();
    void detectTask();

    esp_codec_dev_handle_t microphone_;
    AfeHandle afeHandle_;
    std::pmr::vector<sample_type> microphoneBuffer_;
    AudioBuffer audioBuffer_;
    Task feedTask_;
    Task detectTask_;
    bool volatile running_{true};
};

#endif