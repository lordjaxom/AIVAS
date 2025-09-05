#ifndef AIVAS_IOT_SINGLETON_HPP
#define AIVAS_IOT_SINGLETON_HPP

#include <cassert>

template<typename Derived>
class Singleton
{
public:
    static Derived& get()
    {
        assert(instance_ != nullptr);
        return *instance_;
    }

    Singleton(Singleton const&) = delete;

protected:
    Singleton() noexcept
    {
        instance_ = static_cast<Derived*>(this);
    }

private:
    static Derived* instance_;
};

template<typename Derived>
Derived* Singleton<Derived>::instance_{};

#endif