#include <utility>

#include <portmacro.h>

#include "Context.hpp"

Context::Context(char const* clientId)
    : clientId_(clientId)
{
}

void Context::dispatch(Runnable runnable) const
{
    queue_.emplace(portMAX_DELAY, std::move(runnable));
}

void Context::run() const
{
    for (;;) {
        if (auto const received = queue_.receive(portMAX_DELAY)) {
            (*received)();
        }
    }
}
