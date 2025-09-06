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

#include "Timer.hpp"
#include <bsp/esp-box-3.h>

extern "C" void app_main()
{
    std::pmr::set_default_resource(&psram_memory_resource);

    [[maybe_unused]] Application app{"Office-Aivas-Companion"};
    [[maybe_unused]] WiFi wiFi{"VillaKunterbunt", "sacomoco02047781"};
    [[maybe_unused]] Mqtt mqtt{"openhab"};
    [[maybe_unused]] Display display;
    [[maybe_unused]] AudioSession audioSession;
    [[maybe_unused]] MarvinSession marvinSession;


    Timer radarTimer{"radar", [] {
    }};

    app.run();
}
