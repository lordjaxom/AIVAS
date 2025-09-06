#ifndef AIVAS_IOT_MARVINSESSION_HPP
#define AIVAS_IOT_MARVINSESSION_HPP

#include <optional>

#include "Event.hpp"
#include "Task.hpp"

class MarvinSession
{
public:
    MarvinSession();
    MarvinSession(MarvinSession const&) = delete;

private:
    void streamTask();

    void afeDetected();
    void afeSpeech();
    void afeSilence();

    Subscription afeDetected_;
    Subscription afeSpeech_;
    Subscription afeSilence_;
    std::optional<Task> streamTask_;
    bool volatile running_{};
    std::atomic<bool> streaming_{false};
};

#endif