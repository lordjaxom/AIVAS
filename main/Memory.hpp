#ifndef AIVAS_IOT_MEMORY_HPP
#define AIVAS_IOT_MEMORY_HPP

#include <atomic>
#include <memory_resource>

#include <esp_heap_caps.h>

struct idf_resource_base : std::pmr::memory_resource
{
    virtual void* reallocate(void* ptr, std::size_t new_size) = 0;
};

template<uint32_t Caps, bool Default>
struct idf_resource final : idf_resource_base
{
    std::atomic<std::size_t> alloc_count;
    std::atomic<std::size_t> free_count;

    idf_resource()
    {
        if constexpr (Default) std::pmr::set_default_resource(this);
    }

    void* reallocate(void* ptr, std::size_t const new_size) override
    {
        return heap_caps_realloc(ptr, new_size, Caps | MALLOC_CAP_8BIT);
    }

    void* do_allocate(std::size_t const size, std::size_t) override
    {
        alloc_count += size;
        if (auto const ptr = heap_caps_malloc(size, Caps | MALLOC_CAP_8BIT); ptr != nullptr) return ptr;
        throw std::bad_alloc{};
    }

    void do_deallocate(void* ptr, std::size_t const size, std::size_t) override
    {
        free_count += size;
        heap_caps_free(ptr);
    }

    [[nodiscard]] bool do_is_equal(const memory_resource& other) const noexcept override
    {
        return this == &other;
    }
};

using idf_psram_resource = idf_resource<MALLOC_CAP_SPIRAM, true>;
using idf_internal_resource = idf_resource<MALLOC_CAP_INTERNAL, false>;

extern idf_psram_resource psram_resource;
extern idf_internal_resource internal_resource;

#endif
