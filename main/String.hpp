#ifndef ESPIDF_IOT_STRING_HPP
#define ESPIDF_IOT_STRING_HPP

#include <string>
#include <type_traits>
#include <utility>

#include "Memory.hpp"

using String = std::pmr::string;

namespace detail {
    template<typename T>
    auto str_append(String& result, T arg) ->
        std::enable_if_t<!std::is_convertible_v<T, std::string_view> >
    {
        using std::to_string;
        result += to_string(arg);
    }

    template<typename T>
    auto str_append(String& result, T&& arg) ->
        std::enable_if_t<std::is_convertible_v<T, std::string_view> >
    {
        result += std::forward<T>(arg);
    }

    inline void str(String&) {}

    template<typename Arg0, typename... Args>
    void str(String& result, Arg0&& arg0, Args&&... args)
    {
        str_append(result, std::forward<Arg0>(arg0));
        str(result, std::forward<Args>(args)...);
    }
} // namespace detail

template<typename... Args>
String str(Args&&... args)
{
    String result;
    detail::str(result, std::forward<Args>(args)...);
    return result;
}

#endif
