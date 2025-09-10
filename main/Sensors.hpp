#ifndef AIVAS_RADARSENSOR_HPP
#define AIVAS_RADARSENSOR_HPP

#include <at581x.h>
#include <aht20.h>

#include "Event.hpp"
#include "Singleton.hpp"
#include "Timer.hpp"

class Sensors : public Singleton<Sensors>
{
    static constexpr auto radarSensorGpio = GPIO_NUM_21;

public:
    Sensors();
    Sensors(Sensors const&) = delete;

    [[nodiscard]] bool radarState() const { return radarState_; }
    [[nodiscard]] float temperature() const { return temperature_; }
    [[nodiscard]] float humidity() const { return humidity_; }

    SubscribeEvent<void(bool)> radarStateEvent;

private:
    [[nodiscard]] static i2c_bus_handle_t initExpandI2cBus();
    [[nodiscard]] at581x_dev_handle_t initRadarSensor();
    [[nodiscard]] aht20_dev_handle_t initTempHumSensor() const;

    static void IRAM_ATTR radarSensorIsrHandler(void* arg);

    void readRadarSensor();
    void readTempHumSensors();

    void fireRadarStateEvent();

    i2c_bus_handle_t i2cBus_;
    at581x_dev_handle_t radar_;
    aht20_dev_handle_t tempHum_;
    Timer sensorTimer_;
    bool radarState_{};
    float temperature_{};
    float humidity_{};
};

#endif