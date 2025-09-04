#ifndef AIVAS_IOT_MEMORY_HPP
#define AIVAS_IOT_MEMORY_HPP

#include <atomic>
#include <memory_resource>

#include <esp_heap_caps.h>

struct psram_resource : std::pmr::memory_resource
{
    static std::atomic<std::size_t> alloc_count;

    psram_resource()
    {
        std::pmr::set_default_resource(this);
    }

    void* do_allocate(std::size_t const size, std::size_t) override
    {
        alloc_count += size;
        return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }

    void do_deallocate(void* p, std::size_t, std::size_t) override { heap_caps_free(p); }
    bool do_is_equal(const memory_resource& o) const noexcept override { return this == &o; }
};

#endif
