#ifndef ESPIDF_IOT_EVENT_HPP
#define ESPIDF_IOT_EVENT_HPP

#include "Function.hpp"

#include <list>
#include <memory>
#include <utility>
#include <vector>

template<typename Signature>
class OneShotEvent
{
    using Handler = Function<Signature>;

public:
    OneShotEvent() = default;
    OneShotEvent(OneShotEvent const&) = delete;

    void connect(Handler handler)
    {
        handlers_.push_back(std::move(handler));
    }

    template<typename... Args>
    void operator()(Args&&... args)
    {
        for (auto handlers{std::move(handlers_)};
             auto const& handler: handlers) {
            handler(std::forward<Args>(args)...);
        }
    }

private:
    std::pmr::vector<Handler> handlers_;
};

using Subscription = std::unique_ptr<bool, void (*)(bool* deleted)>;

template<typename Signature>
class SubscribeEvent
{
    using Handler = Function<Signature>;

public:
    SubscribeEvent() = default;
    SubscribeEvent(SubscribeEvent const&) = delete;

    [[nodiscard]] Subscription connect(Handler handler)
    {
        auto it = handlers_.emplace(handlers_.end(), std::move(handler), false);
        return {&it->second, [](auto deleted) { *deleted = true; }};
    }

    template<typename... T>
    void operator()(T&&... args)
    {
        // elements added inside handlers should not be invoked
        auto it = handlers_.begin();
        auto size = handlers_.size();
        for (size_t i = 0; i < size; ++i) {
            // deleted flag might change during invocation
            if (!it->second) {
                it->first(std::forward<T>(args)...);
            }
            if (it->second) {
                it = handlers_.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    std::pmr::list<std::pair<Handler, bool> > handlers_;
};

#endif
