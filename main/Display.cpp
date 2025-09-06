#include <bsp/esp-box-3.h>

#include "Display.hpp"

static constexpr auto TAG = "Display";

Display::Display()
    // : queue_{8}
{
    ESP_ERROR_CHECK(bsp_display_start() ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(bsp_display_brightness_set(100));

    ESP_ERROR_CHECK(bsp_display_lock(0) ? ESP_OK : ESP_FAIL);

    auto const screen = lv_screen_active();
    label_ = lv_label_create(screen);
    lv_obj_set_size(label_, LV_PCT(100), LV_PCT(20));
    lv_label_set_long_mode(label_, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label_, "Hello, I am AIVAS");
    lv_obj_set_style_text_align(label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_, LV_FONT_DEFAULT, 0);
    lv_obj_center(label_);
    bsp_display_unlock();

    task_.emplace("display", Function<void()>{*this, &Display::displayTask}, StackDepth{2048});

    ESP_LOGI(TAG, "display successfully initialized");
}

Display::~Display()
{
    running_ = false;
}
//
// void Display::postText(String text) const
// {
//     queue_.acquire(std::move(text));
// }

void Display::displayTask() const
{
    auto lastRefresh = xTaskGetTickCount();
    while (running_) {
        if (bsp_display_lock(100)) {
            lv_timer_handler();

            // while (auto const received = queue_.receive(Duration::none())) {
            //     lv_label_set_text(label_, received->c_str());
            // }

            bsp_display_unlock();
        }

        xTaskDelayUntil(&lastRefresh, refreshDelay.ticks());
    }
}
