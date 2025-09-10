#include <bsp/esp-box-3.h>

#include "Display.hpp"

#include <functional>
#include <memory>

#include "Memory.hpp"

static constexpr auto TAG = "Display";

LV_IMAGE_DECLARE(background);
LV_IMAGE_DECLARE(body);
LV_IMAGE_DECLARE(body_eye_screen);
LV_IMAGE_DECLARE(body_shadow);
LV_IMAGE_DECLARE(eyes_listen);
LV_IMAGE_DECLARE(eyes_listen_2);
LV_IMAGE_DECLARE(eyes_sleep);
// LV_IMAGE_DECLARE(get_body_eyes);

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

    auto const img_background = lv_img_create(screen);
    lv_img_set_src(img_background, &background);
    lv_obj_align(img_background, LV_ALIGN_TOP_LEFT, 0, 0);
    auto const img_body = lv_img_create(screen);
    lv_img_set_src(img_body, &body);
    lv_obj_align(img_body, LV_ALIGN_TOP_LEFT, 101, 61);
    auto const img_body_shadow = lv_img_create(screen);
    lv_img_set_src(img_body_shadow, &body_shadow);
    lv_obj_align(img_body_shadow, LV_ALIGN_TOP_LEFT, 114, 162);

    img_sleep_ = lv_img_create(screen);
    lv_img_set_src(img_sleep_, &body_eye_screen);
    lv_obj_align(img_sleep_, LV_ALIGN_TOP_LEFT, 140, 110);
    auto const img_eyes_sleep = lv_img_create(img_sleep_);
    lv_img_set_src(img_eyes_sleep, &eyes_sleep);
    lv_obj_align(img_eyes_sleep, LV_ALIGN_TOP_LEFT, 13, 12);

    img_listen_ = lv_img_create(screen);
    lv_img_set_src(img_listen_, &body_eye_screen);
    lv_obj_align(img_listen_, LV_ALIGN_TOP_LEFT, 130, 110);
    auto const img_eyes_listen = lv_img_create(img_listen_);
    lv_img_set_src(img_eyes_listen, &eyes_listen);
    lv_obj_align(img_eyes_listen, LV_ALIGN_TOP_LEFT, 19,10);
    lv_obj_set_flag(img_listen_, LV_OBJ_FLAG_HIDDEN, true);

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
}

void Display::listen() const
{
    bsp_display_lock(0);
    lv_obj_set_flag(img_sleep_, LV_OBJ_FLAG_HIDDEN, true);
    lv_obj_set_flag(img_listen_, LV_OBJ_FLAG_HIDDEN, false);
    bsp_display_unlock();
}

void Display::sleep() const
{
    bsp_display_lock(0);
    lv_obj_set_flag(img_sleep_, LV_OBJ_FLAG_HIDDEN, false);
    lv_obj_set_flag(img_listen_, LV_OBJ_FLAG_HIDDEN, true);
    bsp_display_unlock();
}
