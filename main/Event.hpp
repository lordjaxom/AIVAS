#ifndef ESPIDF_IOT_EVENT_HPP
#define ESPIDF_IOT_EVENT_HPP

#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include <utility>

using Subscription = std::unique_ptr<bool, void (*)(bool* deleted)>;

template<typename Signature, bool oneShot = false>
class Event
{
    using Handler = std::function<Signature>;

public:
    Event() noexcept = default;
    Event(Event const&) = delete;

    Event& operator+=(Handler handler)
    {
        static_assert(oneShot, "Event::operator+= is not allowed for regular events");
        handlers_.emplace_back(std::move(handler), false);
        return *this;
    }

    Subscription subscribe(Handler handler)
    {
        static_assert(!oneShot, "Event::subscribe is not allowed for one-shot events");
        auto it = handlers_.emplace(handlers_.end(), std::move(handler), false);
        return {&it->second, [](auto deleted) { *deleted = true; }};
    }

    void clear()
    {
        handlers_.clear();
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
    std::list<std::pair<Handler, bool> > handlers_;
};

template<typename Signature>
using OneShotEvent = Event<Signature, true>;

#endif
