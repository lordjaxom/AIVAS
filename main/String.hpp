#ifndef ESPIDF_IOT_STRING_HPP
#define ESPIDF_IOT_STRING_HPP

#include <string>
#include <type_traits>
#include <utility>

#include "Memory.hpp"

using String = std::pmr::string;

namespace detail {
    inline void str_append(String& result, std::string_view const& arg)
    {
        result += arg;
    }

    template<typename T>
    requires(!std::is_convertible_v<T, std::string_view>)
    void str_append(String& result, T arg)
    {
        using std::to_string;
        result += to_string(arg);
    }
} // namespace detail

template<typename... Args>
String str(Args&&... args)
{
    String result;
    (detail::str_append(result, std::forward<Args>(args)), ...);
    return result;
}

#endif
