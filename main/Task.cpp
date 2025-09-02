#include "Logger.hpp"
#include "Task.hpp"

static constexpr Logger logger("Task");

Task::Task(char const* name, unsigned const priority, int const core, Runnable runnable)
    : name_{name},
      runnable_{std::move(runnable)}
{
    xTaskCreatePinnedToCore(
        [](auto param) { static_cast<Task*>(param)->run(); },
        name_, 4096, this, priority, &handle_, core
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
    while (eTaskGetState(handle_) != eDeleted) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Task::run() const
{
    logger.info("background task ", name_, " started");

    while (running_) {
        runnable_();
    }
    vTaskDelete(nullptr);
}
