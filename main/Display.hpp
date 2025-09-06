#ifndef AIVAS_IOT_DISPLAY_HPP
#define AIVAS_IOT_DISPLAY_HPP

#include <optional>

#include <misc/lv_types.h>

#include "Singleton.hpp"
#include "Task.hpp"
#include "Time.hpp"

class Display : public Singleton<Display>
{
    static constexpr auto refreshRateHz{25};
    static constexpr auto refreshDelay{Duration::millis(1000 / refreshRateHz)};

public:
    Display();
    Display(Display const&) = delete;
    ~Display();

    // void postText(String text) const;

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
    std::optional<Task> task_;
    bool volatile running_{true};
};

#endif
