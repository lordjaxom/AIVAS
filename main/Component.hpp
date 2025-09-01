#ifndef AIVAS_IOT_COMPONENT_HPP
#define AIVAS_IOT_COMPONENT_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/mp11.hpp>

namespace detail {
    /**
     * @brief Fixed-size string for use in NTTP (non-type template parameters).
     */
    template<std::size_t N>
    struct fixed_string
    {
        char value[N]{};
        // ReSharper disable once CppNonExplicitConvertingConstructor
        constexpr fixed_string(char const (& str)[N]) noexcept { std::copy_n(str, N, value); } // NOLINT(*-explicit-constructor)
    };

    /**
     * @brief Key type for identifying components by type and name.
     */
    template<typename T, auto Name>
    struct component_key
    {
        using value_type = T;
        static constexpr auto name = Name;
    };

    /**
     * @brief Component specification holding type, name and constructor arguments.
     */
    template<typename T, auto Name, typename... Args>
    struct component_spec
    {
        using key_type = component_key<T, Name>;
        using value_type = key_type::value_type;
        static constexpr auto name = Name;
        std::tuple<std::decay_t<Args>...> args;
    };
}

/**
 * @brief Named identifier for components.
 */
template<detail::fixed_string S>
struct named
{
    static constexpr auto value = S;
};

/**
 * @brief Dependency descriptor for components.
 */
template<typename T, auto Name = detail::fixed_string{""}>
struct dep
{
    using key_type = detail::component_key<T, Name>;
    using value_type = key_type::value_type;
    static constexpr auto name = Name; // empty denotes singleton
};

namespace detail {
    /**
     * @brief Helper templates to convert plain types to dep<T> and leave dep<T, Name> unchanged.
     */

    template<typename T>
    struct make_dep
    {
        using type = dep<T>;
    };

    template<typename T, fixed_string Name>
    struct make_dep<dep<T, Name> >
    {
        using type = dep<T, Name>;
    };

    template<typename T>
    using make_dep_t = make_dep<T>::type;
}

/**
 * @brief Component scope enumeration.
 */
enum class Scope
{
    singleton,
    named
};

/**
 * @brief Base class for components to declare scope and dependencies.
 */
template<Scope Scope, typename... Deps>
struct Component
{
    static constexpr auto scope = Scope;
    using deps = boost::mp11::mp_list<detail::make_dep_t<Deps>...>;
};

/**
 * @brief Create a component specification for a singleton component.
 */
template<typename T, typename... Args>
constexpr auto component(Args&&... args) -> detail::component_spec<T, detail::fixed_string{""}, Args...>
{
    static_assert(T::scope == Scope::singleton, "declared component is not a singleton by policy");
    return {{std::forward<Args>(args)...}};
}

/**
 * @brief Create a component specification for a named (non-singleton) component.
 */
template<typename T, auto Name, typename... Args>
constexpr auto component(named<Name>, Args&&... args) -> detail::component_spec<T, Name, Args...>
{
    static_assert(T::scope == Scope::named, "declared component is not a named component by policy");
    return {{std::forward<Args>(args)...}};
}

#endif
