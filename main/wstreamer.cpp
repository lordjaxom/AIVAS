#pragma once
#include <atomic>
#include <functional>
#include <vector>
#include <cstring>
#include "esp_websocket_client.h"
#include "esp_timer.h"
#include "esp_log.h"

// Minimal-Interface deines vorhandenen Ringbuffers (Passe an!)
// Erwartet: drain_last(N, visitor) und pop_nowait(outPtr,outLen)
struct IFrameRing {
    // liefert die letzten N Frames in korrekter Reihenfolge (alt -> neu)
    virtual void drain_last(size_t n, const std::function<void(const uint8_t*, size_t)>& visitor) = 0;
    // pop ohne Blocken; gibt true wenn ein Frame geliefert wurde
    virtual bool pop_nowait(const uint8_t*& data, size_t& len) = 0;
    // wenn pop_nowait zero-copy liefert: caller ruft release() nach dem Senden
    virtual void release(const uint8_t* data) = 0;
    virtual ~IFrameRing() = default;
};

class WsStreamer {
public:
    struct Options {
        const char* host      = "192.168.1.10";
        int         port      = 8080;
        const char* path      = "/ws/audio";
        bool        use_tls   = false;         // wss?
        uint32_t    send_timeout_ms = 200;     // WS send timeout
        bool        keep_open_after_eos = false; // für Folgefragen offen lassen?
        // Heartbeat: Ping alle 15s, 2 verpasste Pongs → reconnect
        uint32_t    hb_interval_ms = 15000;
        uint32_t    hb_max_missed  = 2;
        // Backpressure: max Bytes im ausstehenden Sendepuffer (soft limit)
        size_t      soft_backlog_bytes = 256 * 1024;
    };

    WsStreamer(IFrameRing& ring, Options opt)
        : ring_(ring), opt_(opt) {}

    // ---- Hooks aus AFE-State-Maschine ----
    void detectEvent() {
        // öffne (oder halte) die Verbindung, aber sende noch nicht
        ensure_client();
        ensure_timers();
        start_client_if_needed();
        // Markiere: Armiert, Start wartet auf startEventWithPreroll()
        armed_.store(true, std::memory_order_release);
    }

    void startEventWithPreroll(size_t preroll_frames) {
        if (!is_connected()) return; // optional: queue bis connected
        // Starte Streaming-Phase
        streaming_.store(true, std::memory_order_release);
        // Zuerst Preroll ausschenken
        ring_.drain_last(preroll_frames, [&](const uint8_t* p, size_t n) {
            send_audio_frame(p, n);
        });
        // Danach zieht die loop() live weiter
    }

    void silenceEvent() {
        // EOS senden + optional schließen
        if (is_connected()) {
            static constexpr const char* kEOS = "{\"type\":\"end\"}";
            esp_websocket_client_send_text(ws_, kEOS, strlen(kEOS), pdMS_TO_TICKS(opt_.send_timeout_ms));
            esp_websocket_client_send_ping(ws_); // kleiner Schubs, damit Server flushen kann
        }
        streaming_.store(false, std::memory_order_release);

        if (!opt_.keep_open_after_eos) {
            stop_client();
        } else {
            // Verbindung offen lassen, aber „entwaffnen“
            armed_.store(false, std::memory_order_release);
        }
    }

    // ---- Task-Loop (von dir zyklisch aufgerufen oder in eigenem Task laufen lassen) ----
    void loop() {
        if (!streaming_.load(std::memory_order_acquire)) {
            vTaskDelay(pdMS_TO_TICKS(2));
            return;
        }
        if (!is_connected()) {
            // Lost connection während Stream → sanft neu versuchen
            start_client_if_needed();
            vTaskDelay(pdMS_TO_TICKS(10));
            return;
        }

        // Live-Frames senden, solange vorhanden oder Backpressure erreicht
        size_t sent_this_round = 0;
        const uint8_t* data; size_t len;
        while (ring_.pop_nowait(data, len)) {
            if (!send_audio_frame(data, len)) {
                // Senden schlug fehl → Verbindung wird vermutlich getrennt, abbrechen
                ring_.release(data);
                break;
            }
            sent_this_round += len;
            ring_.release(data);

            if (bytes_inflight_.load(std::memory_order_relaxed) > opt_.soft_backlog_bytes) break;
        }

        if (sent_this_round == 0) {
            // nichts zu tun → kurz schlafen
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }

    // Optional für Debug/Status
    bool is_connected() const {
        return ws_ && esp_websocket_client_is_connected(ws_);
    }

private:
    // ---- WebSocket Grundgerüst ----
    void ensure_client() {
        if (ws_) return;
        char uri[256];
        snprintf(uri, sizeof(uri), "%s://%s:%d%s",
                 opt_.use_tls ? "wss" : "ws", opt_.host, opt_.port, (opt_.path && *opt_.path) ? opt_.path : "/");
        esp_websocket_client_config_t cfg = {};
        cfg.uri = uri;
        ws_ = esp_websocket_client_init(&cfg);
        configASSERT(ws_);
        ESP_ERROR_CHECK(esp_websocket_client_register_event(ws_, ESP_EVENT_ANY_ID, &WsStreamer::ws_event_trampoline, this));
    }

    void ensure_timers() {
        if (!hb_timer_) {
            const esp_timer_create_args_t args{ .callback=&WsStreamer::hb_timer_trampoline, .arg=this, .name="ws_hb" };
            ESP_ERROR_CHECK(esp_timer_create(&args, &hb_timer_));
        }
        if (!retry_timer_) {
            const esp_timer_create_args_t args{ .callback=&WsStreamer::retry_timer_trampoline, .arg=this, .name="ws_retry" };
            ESP_ERROR_CHECK(esp_timer_create(&args, &retry_timer_));
        }
    }

    void start_client_if_needed() {
        if (!ws_) ensure_client();
        if (!is_connected() && !starting_) {
            starting_ = true;
            esp_websocket_client_start(ws_);
        }
    }

    void stop_client() {
        if (ws_) {
            esp_websocket_client_stop(ws_);
            // timers stoppen
            if (hb_timer_) esp_timer_stop(hb_timer_);
            ping_inflight_ = false;
            missed_pongs_  = 0;
        }
        streaming_.store(false);
        armed_.store(false);
    }

    // ---- Senden mit kleinem Header-Hook ----
    bool send_audio_frame(const uint8_t* pcm, size_t n) {
        if (!is_connected()) return false;

        // Optional: deinen eigenen Header voranstellen (seq/timestamp/usw.)
        // Beispiel: [u64 seq][u64 ts][u32 pcm_bytes] + pcm
        // Hier: direkt PCM senden
        int r = esp_websocket_client_send_bin(ws_, reinterpret_cast<const char*>(pcm),
                                              static_cast<int>(n), pdMS_TO_TICKS(opt_.send_timeout_ms));
        if (r < 0) return false;

        bytes_inflight_.fetch_add(n, std::memory_order_relaxed);
        // grobe Abschätzung: nach erfolgreichem Send sofort wieder abziehen (kein echter TX-Pufferzugriff möglich)
        bytes_inflight_.fetch_sub(n, std::memory_order_relaxed);
        return true;
    }

    // ---- Events / Heartbeat ----
    static void ws_event_trampoline(void* arg, esp_event_base_t base, int32_t id, void* data) {
        static_cast<WsStreamer*>(arg)->on_ws_event(base, id, data);
    }

    void on_ws_event(esp_event_base_t, int32_t event_id, void* event_data) {
        auto* e = static_cast<esp_websocket_event_data_t*>(event_data);
        switch (event_id) {
            case WEBSOCKET_EVENT_CONNECTED:
                starting_ = false;
                // starte Heartbeat
                if (hb_timer_) esp_timer_start_periodic(hb_timer_, opt_.hb_interval_ms * 1000);
                missed_pongs_  = 0;
                ping_inflight_ = false;
                ESP_LOGI(TAG, "WS connected");
                break;
            case WEBSOCKET_EVENT_DISCONNECTED:
                starting_ = false;
                if (hb_timer_) esp_timer_stop(hb_timer_);
                ping_inflight_ = false;
                // Reconnect planen
                if (retry_timer_) esp_timer_start_once(retry_timer_, 3000 * 1000);
                ESP_LOGI(TAG, "WS disconnected");
                break;
            case WEBSOCKET_EVENT_DATA:
                // Pong?
                if (e->op_code == 0xA /*PONG*/) {
                    ping_inflight_ = false;
                    missed_pongs_ = 0;
                }
                break;
            default:
                break;
        }
    }

    static void hb_timer_trampoline(void* arg) {
        static_cast<WsStreamer*>(arg)->on_hb_tick();
    }
    void on_hb_tick() {
        if (!is_connected()) return;
        if (ping_inflight_) {
            if (++missed_pongs_ >= opt_.hb_max_missed) {
                ESP_LOGW(TAG, "WS heartbeat missed -> reconnect");
                esp_websocket_client_stop(ws_);
                return; // reconnect via DISCONNECTED
            }
        }
        if (esp_websocket_client_ping(ws_) >= 0) {
            ping_inflight_ = true;
        }
    }

    static void retry_timer_trampoline(void* arg) {
        static_cast<WsStreamer*>(arg)->on_retry();
    }
    void on_retry() {
        if (!is_connected()) start_client_if_needed();
    }

private:
    IFrameRing&        ring_;
    Options            opt_;
    esp_websocket_client_handle_t ws_ = nullptr;
    esp_timer_handle_t hb_timer_ = nullptr;
    esp_timer_handle_t retry_timer_ = nullptr;

    std::atomic<bool>  streaming_{false};
    std::atomic<bool>  armed_{false};
    std::atomic<size_t> bytes_inflight_{0};

    bool               starting_{false};
    bool               ping_inflight_{false};
    uint32_t           missed_pongs_{0};

    static constexpr const char* TAG = "WsStreamer";
};
