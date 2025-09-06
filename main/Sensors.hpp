#ifndef AIVAS_RADARSENSOR_HPP
#define AIVAS_RADARSENSOR_HPP

#include <at581x.h>
#include <aht20.h>

#include "Singleton.hpp"
#include "Task.hpp"

class Sensors : public Singleton<Sensors>
{
public:
    Sensors();
    Sensors(Sensors const&) = delete;

    [[nodiscard]] bool radarState() const;
    [[nodiscard]] float temperature() const { return temperature_; }
    [[nodiscard]] float humidity() const { return humidity_; }

private:
    void sensorTask();

    at581x_dev_handle_t radar_{};
    aht20_dev_handle_t tempHum_{};
    Task sensorTask_;
    bool volatile running_{true};
    float temperature_{};
    float humidity_{};
};

#endif