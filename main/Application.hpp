#ifndef AIVAS_IOT_APPLICATION_HPP
#define AIVAS_IOT_APPLICATION_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

#include <boost/mp11.hpp>

#include "Component.hpp"
#include "Context.hpp"

namespace detail {
    /**
     * @brief Helper template to check for duplicate keys.
     */

    template<typename Keys>
    constexpr bool no_duplicates = std::is_same_v<boost::mp11::mp_unique<Keys>, Keys>;

    /**
     * @brief Helper templates to check if dependencies are present in the key list.
     */

    template<typename Keys>
    struct has_dep
    {
        template<typename Dep>
        using type = boost::mp11::mp_contains<Keys, typename Dep::key_type>;
    };

    template<typename Spec, typename Keys>
    struct check_deps_one
    {
        using present = boost::mp11::mp_transform<has_dep<Keys>::template type, typename Spec::value_type::deps>;
        static constexpr bool value = boost::mp11::mp_all_of<present, boost::mp11::mp_to_bool>::value;
    };

    /**
     * @brief Slot(s) with memory to construct component(s) in-place.
     */

    template<typename Spec>
    struct component_slot
    {
        using type = Spec::value_type;
        alignas(type) std::uint8_t buf[sizeof(type)]{};
        type* ptr{};
    };

    template<typename... Specs>
    using component_slots = std::tuple<component_slot<Specs>...>;

    /**
     * @brief Application private implementation.
     */
    template<typename... Specs>
    class application_impl
    {
        using keys = boost::mp11::mp_list<typename Specs::key_type...>;

        static_assert(std::is_same_v<boost::mp11::mp_unique<keys>, keys>, "found duplicate component specifications");
        static_assert((check_deps_one<Specs, keys>::value && ...), "missing dependencies in component specifications");

        std::tuple<Specs...> specs;
        component_slots<Specs...> slots{};

        static constexpr std::size_t N = sizeof...(Specs);

        template<std::size_t I> using spec_at_k = std::tuple_element_t<I, std::tuple<Specs...> >::key_type;
        template<std::size_t I> using spec_at_v = std::tuple_element_t<I, std::tuple<Specs...> >::value_type;

        template<typename Key, std::size_t I = 0>
        static consteval std::size_t find_index()
        {
            if constexpr (std::is_same_v<Key, spec_at_k<I> >) return I;
            else return find_index<Key, I + 1>();
        }

        template<typename T>
        auto make_dep_tuple() noexcept
        {
            return make_dep_tuple_impl(typename T::deps{});
        }

        template<typename... Deps>
        auto make_dep_tuple_impl(boost::mp11::mp_list<Deps...>) noexcept
        {
            // ReSharper disable once CppRedundantTypenameKeyword
            return std::tuple<typename Deps::value_type&...>(get(typename Deps::key_type{})...);
        }

        template<std::size_t... Is>
        void construct_all(std::index_sequence<Is...>)
        {
            (construct_one<Is>(), ...);
        }

        template<std::size_t I, typename Seen = boost::mp11::mp_list<> >
        void construct_one()
        {
            using T = spec_at_v<I>;
            static_assert(!boost::mp11::mp_contains<Seen, T>::value, "circular dependency detected");
            construct_deps<boost::mp11::mp_push_back<Seen, T> >(typename T::deps{});
            if (auto& slot = std::get<I>(slots); slot.ptr == nullptr) {
                auto& spec = std::get<I>(specs);
                auto deps = make_dep_tuple<T>();
                construct_from(deps, slot, spec.args);
            }
        }

        template<typename Deps, typename Slot, typename Args>
        void construct_from(Deps& deps, Slot& sl, Args& args)
        {
            construct_apply(
                deps, sl, args,
                std::make_index_sequence<std::tuple_size_v<Deps> >{},
                std::make_index_sequence<std::tuple_size_v<Args> >{}
            );
        }

        template<typename Deps, typename Slot, typename Args, std::size_t... DepIs, std::size_t... ArgIs>
        void construct_apply(Deps& deps, Slot& slot, Args& args,
                             std::index_sequence<DepIs...>, std::index_sequence<ArgIs...>)
        {
            slot.ptr = ::new(slot.buf) Slot::type(std::get<DepIs>(deps)..., std::move(std::get<ArgIs>(args))...);
        }

        template<typename Seen, typename... Deps>
        void construct_deps(boost::mp11::mp_list<Deps...>)
        {
            (construct_one<find_index<typename Deps::key_type>(), Seen>(), ...);
        }

        void destroy()
        {
            destroy_all(std::make_index_sequence<N>{});
        }

        template<std::size_t... Is>
        void destroy_all(std::index_sequence<Is...>)
        {
            (destroy_one<Is>(), ...);
        }

        template<std::size_t I>
        void destroy_one()
        {
            if (auto& sl = std::get<I>(slots); sl.ptr) {
                std::destroy_at(reinterpret_cast<spec_at_v<I>*>(sl.ptr));
            }
        }

        template<typename Key>
        Key::value_type& get(Key) noexcept
        {
            return *reinterpret_cast<Key::value_type*>(std::get<find_index<Key>()>(slots).ptr);
        }

    public:
        constexpr explicit application_impl(Specs... s) noexcept
            : specs{std::move(s)...} {}
        application_impl(application_impl const&) = delete;
        ~application_impl() { destroy(); }

        void construct()
        {
            construct_all(std::make_index_sequence<N>{});
        }

        template<typename T, auto Name = fixed_string{""}>
        T& get() noexcept
        {
            return get(component_key<T, Name>{});
        }
    };
}

template<typename... Specs>
class Application
{
    using impl = detail::application_impl<Specs...>;

    static_assert(
        boost::mp11::mp_contains<boost::mp11::mp_list<typename Specs::value_type...>, Context>::value,
        "missing dependency Context in component specifications"
    );

public:
    constexpr explicit Application(Specs... s) noexcept
        : impl_{std::forward<Specs>(s)...}
    {
    }

    void run() noexcept
    {
        impl_.construct();
        impl_.template get<Context>().run();
    }

private:
    impl impl_;
};

template<class... Specs>
constexpr auto make_application(Specs&&... s)
{
    return Application<Specs...>{std::forward<Specs>(s)...};
}

#endif
