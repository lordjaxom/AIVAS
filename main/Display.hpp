#ifndef AIVAS_IOT_DISPLAY_HPP
#define AIVAS_IOT_DISPLAY_HPP

#include <optional>

#include <misc/lv_types.h>

#include "Component.hpp"
#include "Ringbuffer.hpp"
#include "String.hpp"
#include "Task.hpp"
#include "Time.hpp"

class Display : public Component<Scope::singleton>
{
    static constexpr auto refreshDelay{Duration::millis(1000 / 25)}; // 25 Hz

public:
    Display();
    Display(Display const&) = delete;
    ~Display();

    void postText(String text) const;

    // // Helligkeit 0..100
    // void setBrightness(uint8_t percent);
    //
    // // Optional: Bildschirm an/aus (setzt Helligkeit auf 0 / merkt sich letzte Helligkeit)
    // void screenOff();
    // void screenOn();

private:
    void displayTask() const;

    // void applyTextLocked(const char* txt); // nur im UI-Task aufrufen (LVGL locked)

    lv_obj_t* label_{};
    Ringbuffer<String> queue_;
    std::optional<Task> task_;
    bool volatile running_{true};
};

inline auto display()
{
    return component<Display>();
}

#endif
