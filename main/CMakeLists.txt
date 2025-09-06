idf_component_register(
    SRCS
        AIVAS.cpp
#        AfeSession.cpp
#        AfeSession.hpp
        Application.cpp
        Application.hpp
#        Arduino.hpp
        AudioBuffer.cpp
        AudioBuffer.hpp
        AudioSession.cpp
        AudioSession.hpp
        Display.cpp
        Display.hpp
        Event.hpp
        Function.hpp
        Json.cpp
        Json.hpp
        MarvinSession.cpp
        MarvinSession.hpp
        Memory.cpp
        Memory.hpp
#        Microphone.cpp
#        Microphone.hpp
        Mqtt.cpp
        Mqtt.hpp
        Queue.cpp
        Queue.hpp
        Singleton.hpp
        String.hpp
        Task.cpp
        Task.hpp
        Time.hpp
        Timer.cpp
        Timer.hpp
        WebSocket.cpp
        WebSocket.hpp
        WiFi.cpp
        WiFi.hpp
#        WsStreamer.cpp
#        WsStreamer.hpp
    INCLUDE_DIRS .
    REQUIRES
        esp_wifi
        nvs_flash
        esp-box-3
        esp-sr
        esp-boost
        mqtt
        esp_websocket_client
        arduinojson
        at581x
)
