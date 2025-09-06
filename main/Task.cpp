#include <esp_log.h>

#include "Task.hpp"

static constexpr auto TAG{"Task"};

Task::Task(char const* name, uint32_t const stackDepth, unsigned const priority, int const core,
           Runnable const& runnable)
    : name_{name},
      runnable_{runnable}
{
    auto const res = xTaskCreatePinnedToCore(
        [](auto param) { static_cast<Task*>(param)->run(); },
        name_, stackDepth, this, priority, &handle_, core
    );
    assert(res == pdPASS && handle_ != nullptr);
}

Task::~Task()
{
    for (std::size_t waited = 0; waited < shutdownTimeout.millis(); waited += shutdownPollInterval.millis()) {
        if (eTaskGetState(handle_) == eDeleted) return;
        vTaskDelay(shutdownPollInterval.ticks());
    }

    ESP_LOGE(TAG, "background task %s did not exit, killing...", name_);
    vTaskDelete(handle_);
}

void Task::run() const
{
    ESP_LOGI(TAG, "background task %s started", name_);
    runnable_();
    ESP_LOGI(TAG, "background task %s exited", name_);
    vTaskDelete(nullptr);
}
