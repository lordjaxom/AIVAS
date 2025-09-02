#include <utility>

#include <esp_log.h>
#include <portmacro.h>

#include "Context.hpp"

static constexpr auto TAG = "Context";

Context::Context(char const* clientId)
    : clientId_(clientId)
{
}

void Context::dispatch(Runnable runnable) const
{
    queue_.acquire(std::move(runnable));
}

void Context::run() const
{
    ESP_LOGI(TAG, "initialization finished, running event loop");

    for (;;) {
        if (auto const received = queue_.receive()) {
            (*received)();
        }
    }
}
