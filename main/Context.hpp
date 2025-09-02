#ifndef AIVAS_IOT_CONTEXT_HPP
#define AIVAS_IOT_CONTEXT_HPP

#include <functional>

#include "Component.hpp"
#include "Ringbuffer.hpp"

class Context : public Component<Scope::singleton>
{
    using Runnable = std::function<void()>;

    template<typename... Specs> friend class Application;

public:
    explicit Context(char const* clientId);
    Context(Context const&) = delete;

    [[nodiscard]] char const* const& clientId() const { return clientId_; }

    void dispatch(Runnable runnable) const;

private:
    [[noreturn]] void run() const;

    char const* clientId_;
    Ringbuffer<Runnable> queue_{8};
};

inline auto context(char const* clientId)
{
    return component<Context>(clientId);
}

#endif
