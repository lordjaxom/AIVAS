idf_component_register(
        SRCS
            AIVAS.cpp
            Context.cpp
            Display.cpp
            Ringbuffer.cpp
            SoftTimer.cpp
            Task.cpp
            WiFi.cpp
        AIVAS.cpp
        INCLUDE_DIRS .
        REQUIRES
            esp_wifi
            nvs_flash
            esp-box-3
            esp-sr
            esp-boost
)
