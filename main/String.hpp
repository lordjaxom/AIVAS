#ifndef ESPIDF_IOT_STRING_HPP
#define ESPIDF_IOT_STRING_HPP

#include <string>
#include <type_traits>
#include <utility>

namespace detail {
    template<typename T>
    auto str_append(std::string& result, T arg) ->
        std::enable_if_t<std::is_arithmetic_v<T> >
    {
        result += std::to_string(arg);
    }

    template<typename T>
    auto str_append(std::string& result, T&& arg) ->
        std::enable_if_t<
            std::is_same_v<std::decay_t<T>, std::string> ||
            std::is_same_v<std::decay_t<T>, const char*> ||
            std::is_convertible_v<T, std::string_view>
        >
    {
        result += std::forward<T>(arg);
    }

    inline void str(std::string&) {}

    template<typename Arg0, typename... Args>
    void str(std::string& result, Arg0&& arg0, Args&&... args)
    {
        str_append(result, std::forward<Arg0>(arg0));
        str(result, std::forward<Args>(args)...);
    }
} // namespace detail

template<typename... Args>
std::string str(Args&&... args)
{
    std::string result;
    detail::str(result, std::forward<Args>(args)...);
    return result;
}

#endif
