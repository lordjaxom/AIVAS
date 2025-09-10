#ifndef AIVAS_IOT_APPLICATION_HPP
#define AIVAS_IOT_APPLICATION_HPP

#include <string_view>

#include "Function.hpp"
#include "Queue.hpp"
#include "Singleton.hpp"
#include "Timer.hpp"

class Application : public Singleton<Application>
{
    using Runnable = Function<void()>;

public:
    explicit Application(std::string_view clientId) noexcept;

    [[nodiscard]] std::string_view clientId() const { return clientId_; }

    [[noreturn]] void run() const;

    void dispatch(Runnable const& runnable) const;
    void dispatchFromISR(Runnable const& runnable) const;

private:
    static void printMemoryUsage();

    std::string_view clientId_;
    Queue<Runnable> dispatchQueue_{4};
    Timer usageTimer_;
};

#endif
