#ifndef AIVAS_IOT_DISPLAY_HPP
#define AIVAS_IOT_DISPLAY_HPP

#include <optional>
#include <string>

#include "Component.hpp"
#include "Ringbuffer.hpp"
#include "Task.hpp"

class Display : public Component<Scope::singleton>
{
    static constexpr size_t refreshDelay = 40; // 25 Hz

public:
    Display();
    Display(Display const&) = delete;
    ~Display();

    void postText(std::string text) const;

    // // Helligkeit 0..100
    // void setBrightness(uint8_t percent);
    //
    // // Optional: Bildschirm an/aus (setzt Helligkeit auf 0 / merkt sich letzte Helligkeit)
    // void screenOff();
    // void screenOn();

private:
    void uiTask();

    // void applyTextLocked(const char* txt); // nur im UI-Task aufrufen (LVGL locked)

    Ringbuffer<std::string> queue_;
    std::optional<Task> task_;
    uint32_t lastRefresh_{};
    void* label_{};
};

inline auto display()
{
    return component<Display>();
}

#endif
