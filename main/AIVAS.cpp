#include "AfeSession.hpp"
#include "Application.hpp"
#include "Context.hpp"
#include "Display.hpp"
#include "Microphone.hpp"
#include "MqttClient.hpp"
#include "WiFi.hpp"

auto app = make_application(
    context("Office-Aivas-Companion"),
    wiFi("VillaKunterbunt", "sacomoco02047781"),
    mqttClient("openhab"),
    display(),
    microphone(),
    afeSession()
);

extern "C" void app_main()
{
    app.run();
}
