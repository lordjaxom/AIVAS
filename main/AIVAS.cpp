#include "AfeSession.hpp"
#include "Application.hpp"
#include "Context.hpp"
#include "Display.hpp"
#include "Memory.hpp"
#include "Microphone.hpp"
#include "MqttClient.hpp"
#include "WiFi.hpp"
#include "WsStreamer.hpp"

std::atomic<std::size_t> psram_resource::alloc_count;
psram_resource psram;


auto app = make_application(
    context("Office-Aivas-Companion"),
    wiFi("VillaKunterbunt", "sacomoco02047781"),
    mqttClient("openhab"),
    display(),
    microphone(),
    afeSession(),
    wsStreamer()
);

extern "C" void app_main()
{
    app.run();
}
