#include <cstdio>

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_system.h>

#include "Application.hpp"
#include "Memory.hpp"
#include "Time.hpp"

static constexpr auto TAG{"Application"};

Application::Application(std::string_view const clientId) noexcept
    : clientId_{clientId},
      usageTimer_{"usage", &Application::printMemoryUsage}
{
    usageTimer_.start(Duration::millis(5000), true);
}

void Application::run() const
{
    ESP_LOGI(TAG, "initialization finished, running event loop");

    while (true) {
        if (auto const received = dispatchQueue_.receive()) {
            (*received)();
        }
    }
}

void Application::dispatch(Runnable const& runnable) const
{
    dispatchQueue_.emplace(runnable);
}

void Application::dispatchFromISR(Runnable const& runnable) const
{
    (void) dispatchQueue_.sendFromISR(runnable);
}

void Application::printMemoryUsage()
{
    printf("free heap total:      %lu\n", esp_get_free_heap_size());
    printf("min free heap total:  %lu\n", esp_get_minimum_free_heap_size());

    // Interner Heap (8-bit-fähig = für Stacks etc. geeignet)
    printf("free INTERNAL 8bit:   %u\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    printf("largest INTERNAL blk: %u\n", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));

    // PSRAM
    printf("free SPIRAM 8bit:     %u\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    printf("largest SPIRAM blk:   %u\n", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    printf("psram allocs:         %u\n", psram_memory_resource.alloc_count.load());
    printf("psram freed:          %u\n", psram_memory_resource.free_count.load());
    printf("internal allocs:      %u\n", internal_memory_resource.alloc_count.load());
    printf("internal freed:       %u\n", internal_memory_resource.free_count.load());
}
