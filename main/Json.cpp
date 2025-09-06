#include "Json.hpp"

#include <memory>

detail::ResourceAllocator::ResourceAllocator(idf_resource_base* resource)
    : resource_{resource}
{
}

void* detail::ResourceAllocator::allocate(std::size_t const size)
{
    return resource_->allocate(size);
}

void detail::ResourceAllocator::deallocate(void* pointer)
{
    resource_->deallocate(pointer, 0);
}

void* detail::ResourceAllocator::reallocate(void* ptr, size_t const new_size)
{
    return resource_->reallocate(ptr, new_size);
}

ARDUINOJSON_BEGIN_PUBLIC_NAMESPACE
    String to_string(JsonDocument const& json)
    {
        String result;
        serializeJson(json, result);
        return result;
    }
ARDUINOJSON_END_PUBLIC_NAMESPACE

ArduinoJson::JsonDocument jsonDocument()
{
    static detail::ResourceAllocator allocator{&psram_resource};
    return ArduinoJson::JsonDocument{&allocator};
}
