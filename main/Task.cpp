#include <esp_log.h>

#include "Task.hpp"

static constexpr auto TAG{"Task"};

Task::Task(char const* name, unsigned const priority, int const core, Runnable runnable)
    : name_{name},
      runnable_{std::move(runnable)}
{
    auto res = xTaskCreatePinnedToCore(
        [](auto param) { static_cast<Task*>(param)->run(); },
        name_, 4096, this, priority, &handle_, core
    );
    ESP_LOGI(TAG, "background task %s started with result %d", name_, res);
    configASSERT(res == pdPASS && handle_ != nullptr);
}

Task::Task(char const* name, Runnable runnable)
    : Task{name, 2, tskNO_AFFINITY, std::move(runnable)}
{
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
