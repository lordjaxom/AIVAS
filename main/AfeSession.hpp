#ifndef AIVAS_IOT_AFESESSION_HPP
#define AIVAS_IOT_AFESESSION_HPP

#include <optional>
#include <vector>

#include <esp_afe_sr_iface.h>
#include <esp_vad.h>

#include "Context.hpp"
#include "Component.hpp"
#include "Display.hpp"
#include "Microphone.hpp"
#include "Task.hpp"

class AfeSession : public Component<Scope::singleton, Context, Microphone, Display>
{
public:
    explicit AfeSession(Context& context, Microphone& microphone, Display& display);
    AfeSession(AfeSession const&) = delete;
    ~AfeSession();

private:
    void feedTask();
    void detectTask();

    Context& context_;
    Microphone& microphone_;
    Display& display_;
    std::vector<int16_t> buffer_;
    std::optional<Task> feedTask_;
    std::optional<Task> detectTask_;
    esp_afe_sr_iface_t* interface_{};
    esp_afe_sr_data_t* instance_{};
    vad_state_t state_{};
    uint32_t frameKeep_{};
    bool detected_{};
};

inline auto afeSession()
{
    return component<AfeSession>();
}

#endif
