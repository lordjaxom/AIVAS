#ifndef AIVAS_IOT_AFESESSION_HPP
#define AIVAS_IOT_AFESESSION_HPP

#include <optional>
#include <vector>

#include <esp_afe_sr_iface.h>

#include "AudioBuffer.hpp"
#include "Context.hpp"
#include "Component.hpp"
#include "Event.hpp"
#include "Memory.hpp"
#include "Microphone.hpp"
#include "Task.hpp"

class AfeSession : public Component<Scope::singleton, Context, Microphone>
{
    static constexpr uint32_t dropAfterVerifyFrames = 3;
    static constexpr uint32_t silenceFramesToIdle = 40;

    enum class Phase { idle, armed, feeding };

public:
    AfeSession(Context& context, Microphone& microphone);
    AfeSession(AfeSession const&) = delete;
    ~AfeSession();

    [[nodiscard]] AudioBuffer& audioBuffer() { return *audioBuffer_; }

    Event<void()> detectEvent;
    Event<void()> startEvent;
    Event<void()> silenceEvent;

private:
    void feedTask();
    void detectTask();

    Context& context_;
    Microphone& microphone_;
    std::pmr::vector<int16_t> buffer_;
    esp_afe_sr_iface_t* interface_{};
    esp_afe_sr_data_t* instance_{};
    std::optional<AudioBuffer> audioBuffer_;
    std::optional<Task> feedTask_;
    std::optional<Task> detectTask_;
    bool volatile running_{true};
};

inline auto afeSession()
{
    return component<AfeSession>();
}

#endif
