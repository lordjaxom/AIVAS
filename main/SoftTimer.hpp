#ifndef ESPIDF_IOT_SOFTTIMER_HPP
#define ESPIDF_IOT_SOFTTIMER_HPP

#include <cstdint>
#include <functional>

class Context;

class SoftTimer
{
    using Handler = std::function<void()>;

public:
    SoftTimer(Context& context, char const* name, Handler handler);
    SoftTimer(SoftTimer const&) = delete;
    ~SoftTimer();

    [[nodiscard]] bool active() const;

    void start(uint32_t timeout, bool repeat = false) const;
    void stop() const;

private:
    void timedOut() const;

    Context& context_;
    char const* name_;
    std::function<void()> handler_;
    void* handle_;
};

#endif
