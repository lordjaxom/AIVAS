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
#include "Mqtt.hpp"
#include "WiFi.hpp"

extern "C" void app_main()
{
    Application app{"Office-Aivas-Companion"};
    WiFi wiFi{"VillaKunterbunt", "sacomoco02047781"};
    Mqtt mqtt{"openhab"};

    app.run();
}
