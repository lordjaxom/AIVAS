#include <esp_log.h>

#include "Application.hpp"
#include "AudioSession.hpp"
#include "Json.hpp"
#include "MarvinSession.hpp"
#include "WebSocket.hpp"

static constexpr auto TAG{"MarvinSession"};

static bool waitUntil(auto pred, Duration const timeout)
{
    constexpr auto delayMs = 1;

    auto const start = pdTICKS_TO_MS(xTaskGetTickCount());
    while (!pred()) {
        if (pdTICKS_TO_MS(xTaskGetTickCount()) - start >= timeout.millis()) return false;
        vTaskDelay(pdMS_TO_TICKS(delayMs));
    }
    return true;
}

MarvinSession::MarvinSession()
    : afeDetected_{AudioSession::get().detectEvent.connect({*this, &MarvinSession::afeDetected})},
      afeSpeech_{AudioSession::get().speechEvent.connect({*this, &MarvinSession::afeSpeech})},
      afeSilence_{AudioSession::get().silenceEvent.connect({*this, &MarvinSession::afeSilence})}
{
}

void MarvinSession::afeDetected()
{
    running_ = true;
    streamTask_.emplace("marvinStream", fn(*this, &MarvinSession::streamTask), StackDepth{8192}, Priority{5}, Core{0});
}

void MarvinSession::afeSpeech()
{
    AudioSession::get().audioBuffer().drop_except_last(2);
    streaming_.store(true, std::memory_order_release);
}

void MarvinSession::afeSilence()
{
    streaming_.store(false, std::memory_order_release);
}

void MarvinSession::streamTask()
{
    constexpr auto connectTimeout = Duration::millis(10'000);
    constexpr auto streamingTimeout = Duration::millis(500);
    constexpr auto flushWaitDelay = Duration::millis(500);

    std::unique_ptr<std::optional<Task>, void(*)(void*)> scopeGuard{
        &streamTask_, [](auto ptr) {
            ESP_LOGI(TAG, "dispatching deletion of marvin stream task to main loop");
            Application::get().dispatch({*static_cast<std::optional<Task>*>(ptr), &std::optional<Task>::reset});
        }
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

    {
        auto startMsg = jsonDocument();
        startMsg["type"] = "start";
        startMsg["deviceId"] = Application::get().clientId();
        startMsg["fmt"] = "pcm16_le";
        startMsg["sampleRate"] = 16000;
        startMsg["frameSamples"] = 320;
        startMsg["channels"] = 1;
        startMsg["endian"] = "le";
        startMsg["gain"] = 1.0;
        webSocket.sendText(str(startMsg));
    }

    auto& audioBuffer = AudioSession::get().audioBuffer();
    while (running_) {
        if (!streaming_.load(std::memory_order_acquire)) {
            auto endMsg = jsonDocument();
            endMsg["type"] = "stop";
            webSocket.sendText(str(endMsg));
            vTaskDelay(flushWaitDelay.ticks());
            break;
        }

        struct Visitor
        {
            WebSocket& webSocket;
            std::size_t frameSize;

            void operator()(std::int16_t const* ptr) const
            {
                webSocket.sendBinary({reinterpret_cast<uint8_t const*>(ptr), frameSize * sizeof(int16_t) });
            }
        };

        Visitor visitor{webSocket, audioBuffer.frameSize()};
        bool anySent{};
        while (running_ && audioBuffer.pop_nowait({visitor, &Visitor::operator()})) {
            anySent = true;
        }
        if (!anySent) vTaskDelay(1);
    }
}
