#ifndef AIVAS_IOT_JSON_HPP
#define AIVAS_IOT_JSON_HPP

#include <ArduinoJson.hpp>

#include "Memory.hpp"
#include "String.hpp"

namespace detail {
    struct ResourceAllocator final : ArduinoJson::Allocator
    {
        explicit ResourceAllocator(idf_resource_base* resource);
        ~ResourceAllocator() = default;

        void* allocate(size_t size) override;
        void deallocate(void* pointer) override;
        void* reallocate(void* ptr, size_t new_size) override;

    private:
        idf_resource_base* resource_;
    };
}

ARDUINOJSON_BEGIN_PUBLIC_NAMESPACE
    String to_string(JsonDocument const& json);
ARDUINOJSON_END_PUBLIC_NAMESPACE

ArduinoJson::JsonDocument jsonDocument();

#endif
