#ifndef AIVAS_IOT_TASK_HPP
#define AIVAS_IOT_TASK_HPP

#include <functional>

class Task
{
    using Runnable = std::function<void()>;

public:
    Task(char const* name, unsigned priority, int core, Runnable runnable);
    Task(char const* name, Runnable function);
    Task(Task const&) = delete;
    ~Task();

private:
    void run() const;

    char const* name_;
    Runnable runnable_;
    void* handle_{};
    bool volatile running_{true};
};

#endif
