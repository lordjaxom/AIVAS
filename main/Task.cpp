#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Task.hpp"

Task::Task(char const* name, unsigned const priority, int const core, Runnable runnable)
    : name_{name},
      runnable_{std::move(runnable)}
{
    xTaskCreatePinnedToCore(
        [](auto param) { static_cast<Task*>(param)->run(); },
        name_, 4096, this, priority, reinterpret_cast<TaskHandle_t*>(&handle_), core
    );
    configASSERT(handle_ != nullptr);
}

Task::Task(char const* name, Runnable runnable)
    : Task{name, 2, tskNO_AFFINITY, std::move(runnable)}
{
}

Task::~Task()
{
    running_ = false;
    while (eTaskGetState(static_cast<TaskHandle_t>(handle_)) != eDeleted) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void Task::run() const
{
    while (running_) {
        runnable_();
    }
    vTaskDelete(nullptr);
}
