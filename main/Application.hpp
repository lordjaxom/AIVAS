#ifndef AIVAS_IOT_APPLICATION_HPP
#define AIVAS_IOT_APPLICATION_HPP

#include <string_view>

#include "Function.hpp"
#include "Queue.hpp"
#include "Singleton.hpp"

class Application : public Singleton<Application>
{
    using Runnable = Function<void()>;

public:
    explicit Application(std::string_view clientId) noexcept;

    [[nodiscard]] std::string_view clientId() const { return clientId_; }

    [[noreturn]] void run() const;

    void dispatch(Runnable runnable) const;

private:
    std::string_view clientId_;
    Queue<Runnable> dispatchQueue_{4};
};

#endif
