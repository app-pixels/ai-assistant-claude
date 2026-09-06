/*
 * audio_engine.cpp — ES8311 + I2S audio engine with dedicated FreeRTOS task
 *
 * The audio task runs on Core 0, continuously reading I2S RX and writing
 * I2S TX in ~16ms cycles.  It never waits for the main loop.  If no
 * playback data is available it writes silence, keeping the DMA fed and
 * the ES8311 codec happy.
 */

#include "audio_engine.h"
#include "app_common.h"
#include <Arduino.h>
#include <Wire.h>
#include "ESP_I2S.h"
#include "es8311.h"
#include "board.h"
#include "HWCDC.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern USBCDC USBSerial;

// ── Constants ────────────────────────────────────────────────────────────────
#define SAMPLE_RATE      16000
#define I2C_CODEC        0
#define CHUNK            256      // stereo frames per I2S read/write (~16 ms)
#define RING_CAP         32768    // mono samples per ring buffer (~2 s)
#define RING_MASK        (RING_CAP - 1)
#define TASK_STACK       4096
#define TASK_PRIO        5        // above default Arduino loop (1)

// ── Lock-free SPSC ring buffer (one producer, one consumer) ──────────────────
struct Ring {
    int16_t *buf;
    volatile uint32_t head;   // written by producer
    volatile uint32_t tail;   // written by consumer

    bool create() {
        buf = (int16_t *)ps_malloc(RING_CAP * sizeof(int16_t));
        head = tail = 0;
        return buf != nullptr;
    }
    void reset() { head = tail = 0; }
    uint32_t count() const { return head - tail; }
    uint32_t space() const { return RING_CAP - count(); }

    int push(const int16_t *src, int n) {
        uint32_t sp = space();
        if ((uint32_t)n > sp) n = (int)sp;
        uint32_t h = head;
        for (int i = 0; i < n; i++)
            buf[(h + i) & RING_MASK] = src[i];
        __asm__ __volatile__("memw" ::: "memory");
        head = h + n;
        return n;
    }

    int pull(int16_t *dst, int n) {
        uint32_t avail = count();
        if ((uint32_t)n > avail) n = (int)avail;
        uint32_t t = tail;
        for (int i = 0; i < n; i++)
            dst[i] = buf[(t + i) & RING_MASK];
        __asm__ __volatile__("memw" ::: "memory");
        tail = t + n;
        return n;
    }
};

// ── Module state ─────────────────────────────────────────────────────────────
static I2SClass        s_i2s;
static es8311_handle_t s_codec   = nullptr;
static TaskHandle_t    s_task    = nullptr;
static Ring            s_tx;            // main loop → task (playback)
static Ring            s_rx;            // task → main loop (recording)
static volatile bool   s_txActive  = false;
static volatile bool   s_rxActive  = false;
static volatile bool   s_running   = false;
static int             s_sw_volume = 100;    // software volume 0..100, applied in push()
static volatile bool   s_muted     = false;  // mute output (recording)

// ── Audio task (Core 0) ─────────────────────────────────────────────────────
static void audioTask(void *) {
    int16_t stereo[CHUNK * 2];
    int16_t mono[CHUNK];
    uint32_t loopCount = 0;

    s_i2s.setTimeout(50);

    while (s_running) {
        loopCount++;

        // Every ~5 seconds, re-poke critical ES8311 registers
        if (s_codec && (loopCount % 300) == 0) {
            es8311_voice_volume_set(s_codec, 85, nullptr);
        }
        // ── TX: always write something ──────────────────────────────────
        int txN = 0;
        if (s_txActive && !s_muted)
            txN = s_tx.pull(mono, CHUNK);

        if (txN > 0) {
            for (int i = 0; i < txN; i++) {
                stereo[i * 2]     = mono[i];
                stereo[i * 2 + 1] = mono[i];
            }
            s_i2s.write((uint8_t *)stereo, txN * 4);
        } else {
            memset(stereo, 0, CHUNK * 4);
            s_i2s.write((uint8_t *)stereo, CHUNK * 4);
        }

        // ── RX: always drain I2S to keep DMA healthy ──────────────────
        {
            size_t got = s_i2s.readBytes((char *)stereo, CHUNK * 4);
            if (s_rxActive && got >= 4) {
                int pairs = (int)got / 4;
                for (int i = 0; i < pairs; i++)
                    mono[i] = stereo[i * 2];
                s_rx.push(mono, pairs);
            }
        }
    }
    vTaskDelete(nullptr);
}

// ── Public API ──────────────────────────────────────────────────────────────
void audio_engine_init() {
    // Match Waveshare example 15 init order exactly:
    // 1. PA HIGH first  2. I2S begin  3. ES8311 init
    // NO DLDO power cycling — we don't know what those rails power,
    // and example 15 works without it.
    pinMode(PA, OUTPUT);
    digitalWrite(PA, HIGH);           // amp ON and stays ON (like example 15)

    // I2S
    s_i2s.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO, I2S_MCK_IO);
    if (!s_i2s.begin(I2S_MODE_STD, SAMPLE_RATE,
                     I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO,
                     I2S_STD_SLOT_BOTH))
        USBSerial.println("[audio] I2S begin FAILED");

    delay(50);                        // MCLK → ES8311 PLL lock

    // ES8311 codec
    s_codec = es8311_create(I2C_CODEC, ES8311_ADDRRES_0);
    if (s_codec) {
        const es8311_clock_config_t clk = {
            .mclk_inverted      = false,
            .sclk_inverted      = false,
            .mclk_from_mclk_pin = true,
            .mclk_frequency     = SAMPLE_RATE * 256,
            .sample_frequency   = SAMPLE_RATE,
        };
        esp_err_t r = es8311_init(s_codec, &clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
        es8311_sample_frequency_config(s_codec, clk.mclk_frequency, clk.sample_frequency);
        es8311_microphone_config(s_codec, false);
        es8311_voice_volume_set(s_codec, 85, nullptr);
        es8311_microphone_gain_set(s_codec, (es8311_mic_gain_t)5);
        USBSerial.printf("[audio] ES8311 init: %s\n", r == ESP_OK ? "OK" : "FAIL");
    } else {
        USBSerial.println("[audio] ES8311 create FAILED");
    }

    // Prime the DMA with silence so it's ready
    {
        int16_t silence[512] = {};
        s_i2s.write((uint8_t *)silence, sizeof(silence));
    }

    // Ring buffers (PSRAM)
    if (!s_tx.create()) USBSerial.println("[audio] TX ring alloc failed");
    if (!s_rx.create()) USBSerial.println("[audio] RX ring alloc failed");

    // Start audio task on Core 0
    s_running  = true;
    s_txActive = false;
    s_rxActive = false;
    xTaskCreatePinnedToCore(audioTask, "audio", TASK_STACK, nullptr,
                            TASK_PRIO, &s_task, 0);

    // Confirmation beep (100 ms, 1 kHz) — PA already HIGH
    {
        const int N = SAMPLE_RATE / 10;
        const int HP = SAMPLE_RATE / 1000 / 2;
        int16_t beep[256];
        s_txActive = true;
        for (int s = 0; s < N; s += 256) {
            int chunk = (s + 256 <= N) ? 256 : N - s;
            for (int i = 0; i < chunk; i++)
                beep[i] = (((s + i) / HP) & 1) ? 16000 : -16000;
            audio_engine_push(beep, chunk);
        }
        delay(200);
        s_txActive = false;
        // PA stays HIGH — never toggle it (matches example 15)
    }

    USBSerial.println("[audio] engine ready");
}

void audio_engine_deinit() {
    s_running = false;
    vTaskDelay(pdMS_TO_TICKS(200));
    s_task = nullptr;
    s_i2s.end();
    if (s_codec) { es8311_delete(s_codec); s_codec = nullptr; }
    digitalWrite(PA, LOW);
}

void audio_engine_play() {
    s_tx.reset();
    s_txActive = true;
    // PA stays HIGH always — no toggling
}

void audio_engine_stop() {
    s_txActive = false;
    // PA stays HIGH always — task writes silence when not active
}

void audio_engine_suspend() {
    if (s_task) vTaskSuspend(s_task);
}

void audio_engine_resume() {
    if (s_task) vTaskResume(s_task);
}

void audio_engine_record() {
    s_rx.reset();
    s_rxActive = true;
}

void audio_engine_record_stop() {
    s_rxActive = false;
}

int audio_engine_push(const int16_t *mono, int count) {
    if (count <= 0) return 0;
    if (s_sw_volume < 100) {
        // Scale samples in a temp buffer before pushing
        int16_t scaled[256];
        int pushed = 0;
        while (pushed < count) {
            int chunk = count - pushed;
            if (chunk > 256) chunk = 256;
            for (int i = 0; i < chunk; i++)
                scaled[i] = (int16_t)((int32_t)mono[pushed + i] * s_sw_volume / 100);
            pushed += s_tx.push(scaled, chunk);
            if (s_tx.space() == 0) break;
        }
        return pushed;
    }
    return s_tx.push(mono, count);
}

int audio_engine_pull(int16_t *mono, int count) {
    if (count <= 0) return 0;
    return s_rx.pull(mono, count);
}

int audio_engine_tx_space() { return (int)s_tx.space(); }
int audio_engine_rx_avail() { return (int)s_rx.count(); }

void audio_engine_set_volume(int vol) {
    if (vol < 10)  vol = 10;
    if (vol > 100) vol = 100;
    s_sw_volume = vol;
}

int audio_engine_get_volume() { return s_sw_volume; }

void audio_engine_mute()   { s_muted = true; }
void audio_engine_unmute() { s_muted = false; }
