#include "bsp/esp-box-3.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_codec_dev.h"
#include "esp_log.h"

static const char* TAG = "AUDIO";
static esp_codec_dev_handle_t s_mic = NULL;   // ES7210
static esp_codec_dev_handle_t s_spk = NULL;   // ES8311 (optional)

static void audio_init(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);              // deine sdkconfig hatte „NONE“

    // BOX-3: getrennte Inits
    s_mic = bsp_audio_codec_microphone_init();
    assert(s_mic);
    s_spk = bsp_audio_codec_speaker_init();            // optional
    assert(s_spk);

    // Format setzen & Pfad starten
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel         = 2,                          // 2 Mics (interleaved)
        .sample_rate     = 16000,
    };
    ESP_ERROR_CHECK(esp_codec_dev_open(s_mic, &fs));
    ESP_ERROR_CHECK(esp_codec_dev_open(s_spk, &fs));   // optional

    // Mic aktivieren
    esp_codec_dev_set_in_mute(s_mic, false);
    esp_codec_dev_set_in_gain(s_mic, 30.0f);           // 0..100 (%), ggf. anpassen

    // Lautsprecher (für spätere TTS)
    esp_codec_dev_set_out_vol(s_spk, 60.0f);
}

static const esp_afe_sr_iface_t* s_afe_if = NULL;
static esp_afe_sr_data_t*        s_afe    = NULL;

static bool afe_init(void)
{
    // 1) Modelle aus „model“-Partition mappen
    srmodel_list_t* models = esp_srmodel_init("model");
    if (!models) {
        ESP_LOGE(TAG, "esp_srmodel_init('model') failed – check partition & Kconfig (WakeNet9 Computer)");
        return false;
    }

    ESP_LOGI("MODEL","%d models loaded from 'model' partition", models->num);
    for (int i = 0; i < models->num; ++i) {
        ESP_LOGI("MODEL", "[%d] name=%s info=%s", i, models->model_name[i], (int)models->model_info[i]);
    }

    // 2) AFE-Config v2 API
    //   Layout: "MMNR" = Mic, Mic, Null, Ref  (wenn du keine Ref/AEC nutzt → "MM" oder "M")
    afe_config_t* cfg = afe_config_init("MM", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    if (!cfg) { ESP_LOGE(TAG, "afe_config_init failed"); return false; }

    // WebRTC NS/VAD sind in deinem sdkconfig aktiv → Feintuning (optional)
    // cfg->ns_mode  = 2;          // 0..3
    // cfg->vad_mode = 3;          // 0..3
    cfg->aec_init = false;      // true, wenn du eine Referenz einspeist

    // 3) Handle & Instance
    s_afe_if = esp_afe_handle_from_config(cfg);
    if (!s_afe_if) { ESP_LOGE(TAG, "esp_afe_handle_from_config failed"); return false; }

    s_afe = s_afe_if->create_from_config(cfg);
    if (!s_afe) { ESP_LOGE(TAG, "AFE create_from_config failed"); return false; }

    // 4) WakeNet aktivieren
    s_afe_if->enable_wakenet(s_afe);

    // 5) Feed/Fetch-Parameter loggen (DARAN die Read-Größe ausrichten!)
    int feed_ch  = s_afe_if->get_feed_channel_num(s_afe);
    int feed_ns  = s_afe_if->get_feed_chunksize(s_afe);
    int fetch_ns = s_afe_if->get_fetch_chunksize(s_afe);
    ESP_LOGI(TAG, "AFE feed: ch=%d, ns/ch=%d (bytes=%d) | fetch ns=%d",
             feed_ch, feed_ns, feed_ch*feed_ns*2, fetch_ns);

    afe_config_print(cfg);

    return true;
}

void app_main()
{
    audio_init();
    if (!afe_init()) return;

    // Puffer gemäß AFE-Vorgabe anlegen
    const int feed_ch  = s_afe_if->get_feed_channel_num(s_afe);
    const int feed_ns  = s_afe_if->get_feed_chunksize(s_afe);
    const size_t feed_bytes = (size_t)feed_ch * feed_ns * sizeof(int16_t);

    int16_t* feed_buf = (int16_t*)(heap_caps_malloc(feed_bytes, MALLOC_CAP_DEFAULT));
    assert(feed_buf);

    ESP_LOGI(TAG, "Listening for wake word \"Computer\"…");

    while (1) {
        ESP_ERROR_CHECK(esp_codec_dev_read(s_mic, feed_buf, feed_bytes));

        // interleaved 2-ch in feed() → die AFE weiß, wie viele Kanäle sie erwartet
        s_afe_if->feed(s_afe, feed_buf);

        afe_fetch_result_t* res = s_afe_if->fetch(s_afe);
        if (!res) continue;

        if (res->wakeup_state == WAKENET_DETECTED) {
            ESP_LOGI(TAG, "Wake word detected!");
            // Hier: Listening-State aktivieren, PCM aus res->data weiterreichen etc.
        }
        // res->data: mono, bereits durch NS/VAD (und ggf. AEC) gelaufen
        // fetch_ns = s_afe_if->get_fetch_chunksize(s_afe) → samples in res->data
    }
}