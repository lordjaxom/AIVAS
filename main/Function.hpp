#ifndef AIVAS_IOT_FUNCTION_HPP
#define AIVAS_IOT_FUNCTION_HPP

#include <cstddef>
#include <algorithm>
#include <array>

template<typename Signature>
class Function;

template<typename Ret, typename... Args>
class Function<Ret(Args...)>
{
    static constexpr std::size_t maxMemberPtrSize = sizeof(void*) * 2;

    using free_fn_type = Ret (*)(Args...);
    using stub_fn_type = Ret (*)(void* free_or_mem_obj, void const* mem, Args&&... args);

    static Ret free_fn_stub(void* free_or_mem_obj, void const*, Args&&... args)
    {
        return reinterpret_cast<Ret(*)(Args...)>(free_or_mem_obj)(std::forward<Args>(args)...);
    }

    template<typename T, typename MemFn>
    static Ret mem_fn_stub(void* free_or_mem_obj, void const* mem, Args&&... args)
    {
        auto pmf = *static_cast<MemFn const*>(mem);
        return (static_cast<T*>(free_or_mem_obj)->*pmf)(std::forward<Args>(args)...);
    }

public:
    Function() noexcept
        : free_fn_or_mem_fn_obj{},
          stub_fn{}
    {
    }

    // ReSharper disable once CppNonExplicitConvertingConstructor
    Function(free_fn_type const function) noexcept // NOLINT(*-explicit-constructor)
        : free_fn_or_mem_fn_obj{reinterpret_cast<void*>(function)},
          stub_fn{&free_fn_stub}
    {
    }

    template<typename F>
    requires(std::is_convertible_v<std::remove_cvref_t<F>, free_fn_type>)
    // ReSharper disable once CppNonExplicitConvertingConstructor
    Function(F&& function) noexcept // NOLINT(*-explicit-constructor)
        : Function{static_cast<free_fn_type>(function)}
    {
    }

    template<typename T>
    Function(T const& object, Ret (T::* member)(Args...) const) noexcept
        : free_fn_or_mem_fn_obj{const_cast<T*>(&object)},
          stub_fn{&mem_fn_stub<T const, Ret (T::*)(Args...) const>}
    {
        static_assert(sizeof(member) <= maxMemberPtrSize, "member function pointer too large");
        // ReSharper disable once CppDFANotInitializedField
        std::copy_n(reinterpret_cast<std::byte*>(&member), sizeof(member), mem_fn.data());
    }

    template<typename T>
    Function(T& object, Ret (T::* member)(Args...)) noexcept
        : free_fn_or_mem_fn_obj{&object},
          stub_fn{&mem_fn_stub<T, Ret (T::*)(Args...)>}
    {
        static_assert(sizeof(member) <= maxMemberPtrSize, "member function pointer too large");
        // ReSharper disable once CppDFANotInitializedField
        std::copy_n(reinterpret_cast<std::byte*>(&member), sizeof(member), mem_fn.data());
    }

    Ret operator()(Args... args) const
    {
        return stub_fn(free_fn_or_mem_fn_obj, mem_fn.data(), std::forward<decltype(args)>(args)...);
    }

private:
    void* free_fn_or_mem_fn_obj;
    alignas(std::max_align_t) std::array<std::byte, maxMemberPtrSize> mem_fn{};
    stub_fn_type stub_fn;
};

/**
 * @brief Helpers to create a Function object from a member function pointer.
 */

template<typename T, typename Ret, typename... Args>
auto fn(T& object, Ret (T::*member)(Args...)) noexcept
{
    return Function<Ret(Args...)>{object, member};
}

template<typename T, typename Ret, typename... Args>
auto fn(T const& object, Ret (T::*member)(Args...) const) noexcept
{
    return Function<Ret(Args...)>{object, member};
}

#endif