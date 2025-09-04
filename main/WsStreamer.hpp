#ifndef AIVAS_IOT_WSSTREAMER_HPP
#define AIVAS_IOT_WSSTREAMER_HPP

#include <atomic>
#include <optional>

#include "AfeSession.hpp"
#include "Component.hpp"
#include "Event.hpp"
#include "Task.hpp"

class WsStreamer : public Component<Scope::singleton, Context, AfeSession>
{
    static constexpr std::size_t prerollFrames = 2;

    enum class Phase { idle, starting, streaming };

public:
    WsStreamer(Context& context, AfeSession& afeSession);

private:
    void afeDetected();
    void afeStarted();
    void afeSilenced();

    void streamTask();
    bool waitForWsConnect();

    Context& context_;
    AudioBuffer& audioBuffer_;
    Subscription afeDetected_;
    Subscription afeStarted_;
    Subscription afeSilenced_;
    std::optional<Task> streamTask_;
    bool volatile running_{false};
    std::atomic<bool> streaming_{false};
};

inline auto wsStreamer()
{
    return component<WsStreamer>();
}

#endif
