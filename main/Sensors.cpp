#include "Application.hpp"
#include "Sensors.hpp"

Sensors::Sensors()
    : i2cBus_{initExpandI2cBus()},
      radar_{initRadarSensor()},
      tempHum_{initTempHumSensor()},
      sensorTimer_{"sensors", {*this, &Sensors::readTempHumSensors}}
{
    readRadarSensor();
    readTempHumSensors();
    sensorTimer_.start(Duration::millis(1000), true);
}

i2c_bus_handle_t Sensors::initExpandI2cBus()
{
    constexpr i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_41,
        .scl_io_num = GPIO_NUM_40,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master{
            .clk_speed = 100000
        },
        .clk_flags = 0
    };
    auto const handle = i2c_bus_create(I2C_NUM_0, &config);
    assert(handle != nullptr);
    return handle;
}

at581x_dev_handle_t Sensors::initRadarSensor()
{
    at581x_default_cfg_t defaults = ATH581X_INITIALIZATION_CONFIG();
    defaults.trig_base_tm_cfg = 1000;
    defaults.trig_keep_tm_cfg = 5000;
    defaults.delta_cfg = 100;
    defaults.gain_cfg = AT581X_STAGE_GAIN_4;
    defaults.power_cfg = AT581X_POWER_91uA;
    at581x_i2c_config_t const config = {
        .bus_inst = i2cBus_,
        .i2c_addr = AT581X_ADDRRES_0, // ggf. auf *_1 ändern, je nach Board
        .int_gpio_num = radarSensorGpio,
        .interrupt_level = 1,
        .interrupt_callback = nullptr,
        .def_conf = &defaults,
    };
    at581x_dev_handle_t handle;
    ESP_ERROR_CHECK(at581x_new_sensor(&config, &handle));

    constexpr gpio_config_t gpio = {
        .pin_bit_mask = 1ULL << radarSensorGpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&gpio));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(radarSensorGpio, &Sensors::radarSensorIsrHandler, this));

    return handle;
}

aht20_dev_handle_t Sensors::initTempHumSensor() const
{
    aht20_i2c_config_t const i2c_conf = {
        .bus_inst = i2cBus_,
        .i2c_addr = AHT20_ADDRRES_0,
    };
    aht20_dev_handle_t handle;
    ESP_ERROR_CHECK(aht20_new_sensor(&i2c_conf, &handle));
    return handle;
}

void Sensors::radarSensorIsrHandler(void* arg)
{
    auto& self = *static_cast<Sensors*>(arg);
    self.readRadarSensor();
    Application::get().dispatchFromISR({self, &Sensors::fireRadarStateEvent});
}

void Sensors::readRadarSensor()
{
    radarState_ = gpio_get_level(radarSensorGpio);
}

void Sensors::readTempHumSensors()
{
    std::uint32_t temp_raw;
    std::uint32_t RH_raw;
    aht20_read_temperature_humidity(tempHum_, &temp_raw, &temperature_, &RH_raw, &humidity_);
}

void Sensors::fireRadarStateEvent()
{
    radarStateEvent(radarState_);
}
