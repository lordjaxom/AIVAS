#include <bsp/esp-box-3.h>

#include "Display.hpp"

#include <functional>
#include <memory>

#include "Memory.hpp"

static constexpr auto TAG = "Display";

Display::Display()
// : queue_{8}
{
    constexpr bsp_display_cfg_t config{
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT,
        .double_buffer = false,
        .flags = {
            .buff_dma = false,
            .buff_spiram = true
        }
    };
    ESP_ERROR_CHECK(bsp_display_start_with_config(&config) ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(bsp_display_brightness_set(100));

    ESP_ERROR_CHECK(bsp_display_lock(0) ? ESP_OK : ESP_FAIL);

    auto const screen = lv_screen_active();
    label_ = lv_label_create(screen);
    lv_obj_set_size(label_, LV_PCT(100), LV_PCT(20));
    lv_label_set_long_mode(label_, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label_, "");
    lv_obj_set_style_text_align(label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_, LV_FONT_DEFAULT, 0);
    lv_obj_center(label_);
    bsp_display_unlock();

    ESP_LOGI(TAG, "display successfully initialized");
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void Display::brightness(int const level) // NOLINT(*-convert-member-functions-to-static)
{
    ESP_ERROR_CHECK(bsp_display_brightness_set(level));
}

void Display::showText(String const& text) const
{
    bsp_display_lock(0);
    lv_label_set_text(label_, text.c_str());
    bsp_display_unlock();
}
