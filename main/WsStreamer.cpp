#include <experimental/scope>

#include <esp_log.h>
#include <nlohmann/json.hpp>

#include "Arduino.hpp"
#include "WebSocket.hpp"
#include "WsStreamer.hpp"

static constexpr auto TAG{"WsStreamer"};

static bool waitUntil(auto pred, Duration const timeout)
{
    constexpr auto delay = Duration::millis(1);
    auto const start = millis();
    while (!pred()) {
        if (millis() - start >= timeout.millis()) return false;
        vTaskDelay(delay.ticks());
    }
    return true;
}

WsStreamer::WsStreamer(Context& context, AfeSession& afeSession)
    : context_{context},
      audioBuffer_{afeSession.audioBuffer()},
      afeDetected_{afeSession.detectEvent.subscribe([this] { afeDetected(); })},
      afeStarted_{afeSession.startEvent.subscribe([this] { afeStarted(); })},
      afeSilenced_{afeSession.silenceEvent.subscribe([this] { afeSilenced(); })}
{
}

void WsStreamer::afeDetected()
{
    streamTask_.emplace("wsStreamer", 5, 0, [this] { streamTask(); });
}

void WsStreamer::afeStarted()
{
    audioBuffer_.drop_except_last(prerollFrames);
    streaming_.store(true, std::memory_order_release);
}

void WsStreamer::afeSilenced()
{
    streaming_.store(false, std::memory_order_release);
}

void WsStreamer::streamTask()
{
    constexpr auto connectTimeout = Duration::millis(10'000);
    constexpr auto streamingTimeout = Duration::millis(500);
    constexpr auto flushWaitDelay = Duration::millis(500);

    auto cleanupGuard = std::experimental::scope_exit{
        [this] { context_.dispatch([this] { streamTask_.reset(); }); }
    };

    WebSocket webSocket{"192.168.176.220", 9090, "/realtime"};
    if (!waitUntil([&webSocket] { return webSocket.connected(); }, connectTimeout)) {
        ESP_LOGE(TAG, "could not connect to WebSocket within %lu ms", connectTimeout.millis());
        return;
    }

    if (!waitUntil([this] { return streaming_.load(std::memory_order_acquire); }, streamingTimeout)) {
        ESP_LOGE(TAG, "no start signal from afe session within %lu ms", streamingTimeout.millis());
        return;
    }

    webSocket.sendText(nlohmann::json{
        {"type", "start"},
        {"deviceId", context_.clientId()},
        {"fmt", "pcm16_le"},
        {"sampleRate", 16000},
        {"frameSamples", 320}, // TODO
        {"channels", 1},
        {"endian", "le"},
        {"gain", 1.0}
    }.dump());

    auto itemSize = audioBuffer_.itemSize();
    while (running_) {
        if (!streaming_.load(std::memory_order_acquire)) {
            webSocket.sendText(nlohmann::json{{"type", "end"}}.dump());
            vTaskDelay(flushWaitDelay.ticks());
            break;
        }

        auto visitor = [&webSocket, itemSize](auto data) { webSocket.sendBinary({data, itemSize}); };
        bool anySent{};
        while (running_ && audioBuffer_.pop_nowait(visitor)) {
            anySent = true;
        }
        if (!anySent) vTaskDelay(1);
    }
}
