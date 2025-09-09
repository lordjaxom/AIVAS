#include "Sensors.hpp"

static constexpr auto radarGpioNum = GPIO_NUM_21;

static i2c_bus_handle_t initExpandI2c()
{
    static i2c_bus_handle_t handle;
    if (handle == nullptr) {
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
        handle = i2c_bus_create(I2C_NUM_0, &config);
        assert(handle != nullptr);
    }
    return handle;
}

static at581x_dev_handle_t initRadarSensor()
{
    static at581x_dev_handle_t handle;
    if (handle == nullptr) {
        at581x_default_cfg_t defaults = ATH581X_INITIALIZATION_CONFIG();
        defaults.trig_base_tm_cfg = 1000;
        defaults.trig_keep_tm_cfg = 5000;
        defaults.delta_cfg = 100;
        defaults.gain_cfg = AT581X_STAGE_GAIN_4;
        defaults.power_cfg = AT581X_POWER_91uA;
        at581x_i2c_config_t const config = {
            .bus_inst = initExpandI2c(),
            .i2c_addr = AT581X_ADDRRES_0, // ggf. auf *_1 ändern, je nach Board
            .int_gpio_num = radarGpioNum,
            .interrupt_level = 1,
            .interrupt_callback = nullptr,
            .def_conf = &defaults,
        };
        ESP_ERROR_CHECK(at581x_new_sensor(&config, &handle));

        constexpr gpio_config_t gpio = {
            .pin_bit_mask = 1ULL << radarGpioNum,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_ANYEDGE
        };
        ESP_ERROR_CHECK(gpio_config(&gpio));
        // ESP_ERROR_CHECK(gpio_isr_handler_add(RADAR_OUT, radar_isr, (void*)(uintptr_t)RADAR_OUT));
    }
    return handle;
}

static aht20_dev_handle_t initTempHumSensor()
{
    static aht20_dev_handle_t handle;
    if (handle == nullptr) {
        aht20_i2c_config_t const i2c_conf = {
            .bus_inst = initExpandI2c(),
            .i2c_addr = AHT20_ADDRRES_0,
        };
        ESP_ERROR_CHECK(aht20_new_sensor(&i2c_conf, &handle));
    }
    return handle;
}

Sensors::Sensors()
    : radar_{initRadarSensor()},
      tempHum_{initTempHumSensor()},
      sensorTask_{"sensors", {*this, &Sensors::sensorTask}, StackDepth{8192}, Priority{5}, Core{1}}
{
}

void Sensors::sensorTask()
{
    while (running_) {
        std::uint32_t temp_raw;
        std::uint32_t RH_raw;
        aht20_read_temperature_humidity(tempHum_, &temp_raw, &temperature_, &RH_raw, &humidity_);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

bool Sensors::radarState() const
{
    return gpio_get_level(radarGpioNum);
}
