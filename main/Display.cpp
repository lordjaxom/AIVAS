#include <bsp/esp-box-3.h>

#include "Display.hpp"

static constexpr auto TAG = "Display";

Display::Display()
    : queue_{8},
      lastRefresh_{xTaskGetTickCount()}
{
    ESP_ERROR_CHECK(bsp_display_start() ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(bsp_display_brightness_set(100));

    ESP_ERROR_CHECK(bsp_display_lock(0) ? ESP_OK : ESP_FAIL);

    auto const screen = lv_screen_active();
    auto const label = lv_label_create(screen);
    lv_obj_set_size(label, LV_PCT(100), LV_PCT(20));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, "Hello, I am AIVAS");
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);
    lv_obj_center(label);
    bsp_display_unlock();
    label_ = label;

    task_.emplace("ui", [this] { uiTask(); });

    ESP_LOGI(TAG, "display successfully initialized");
}

void Display::postText(std::string text) const
{
    queue_.acquire(std::move(text));
}

void Display::uiTask()
{
    if (bsp_display_lock(100)) {
        lv_timer_handler();

        while (auto const received = queue_.receive(Duration::none())) {
            lv_label_set_text(static_cast<lv_obj_t*>(label_), received->c_str());
        }

        bsp_display_unlock();
    }

    xTaskDelayUntil(&lastRefresh_, refreshDelay.ticks());
}
