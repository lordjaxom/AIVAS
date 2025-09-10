// auto app = make_application(
//     context("Office-Aivas-Companion"),
//     wiFi("VillaKunterbunt", "sacomoco02047781"),
//     mqttClient("openhab"),
//     display(),
//     microphone(),
//     afeSession(),
//     wsStreamer()
// );

#include "Application.hpp"
#include "AudioSession.hpp"
#include "Display.hpp"
#include "MarvinSession.hpp"
#include "Memory.hpp"
#include "Mqtt.hpp"
#include "WiFi.hpp"

#include "Sensors.hpp"
#include "Timer.hpp"
#include "esp_log.h"

extern "C" void app_main()
{
    std::pmr::set_default_resource(&psram_memory_resource);

    [[maybe_unused]] Application app{"Office-Aivas-Companion"};
    [[maybe_unused]] WiFi wiFi{"VillaKunterbunt", "sacomoco02047781"};
    [[maybe_unused]] Mqtt mqtt{"openhab"};
    [[maybe_unused]] Sensors sensors;
    [[maybe_unused]] Display display;
    [[maybe_unused]] AudioSession audioSession;
    [[maybe_unused]] MarvinSession marvinSession;

    auto subscription{sensors.radarStateEvent.connect([](bool const state) {
        ESP_LOGI("AIVAS", "radar sensor state changed to %d", state ? 1 : 0);
    })};

    Timer radarTimer{"radar", [] {
        ESP_LOGI("AIVAS", "radar sensor state is %d, temperature %f, hum %f", Sensors::get().radarState(),
            Sensors::get().temperature(), Sensors::get().humidity());
        if (Sensors::get().radarState()) {
            Display::get().brightness(100);
            Display::get().listen();
        } else {
            Display::get().brightness(10);
            Display::get().sleep();
        }
    }};
    radarTimer.start(Duration::millis(1000), true);

    app.run();
}
