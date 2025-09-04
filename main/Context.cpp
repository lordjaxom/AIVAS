#include <utility>

#include <esp_log.h>

#include "Context.hpp"
#include "Memory.hpp"
#include "String.hpp"

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

    String test;
    test.resize(2000);

    while (true) {
        printf("free heap total:      %lu\n", esp_get_free_heap_size());
        printf("min free heap total:  %lu\n", esp_get_minimum_free_heap_size());

        // Interner Heap (8-bit-fähig = für Stacks etc. geeignet)
        printf("free INTERNAL 8bit:   %u\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        printf("largest INTERNAL blk: %u\n", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));

        // PSRAM
        printf("free SPIRAM 8bit:     %u\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        printf("largest SPIRAM blk:   %u\n", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

        printf("resource allocs:      %u\n", psram_resource::alloc_count.load());

        if (auto const received = queue_.receive(Duration::millis(5000))) {
            (*received)();
        }
    }
}
