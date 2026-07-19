// ===========================================================================
//  audio.cpp — Som pelo ES8311 (I2S legado, driver/i2s.h — core 2.0.17).
//
//  Base identica ao projeto "arkade" (tom bloqueante). Aqui foi acrescentado
//  o sidetone: feedKey() escreve um pequeno bloco de senoide a cada chamada,
//  mantendo a fase entre blocos, para sustentar o tom sem bloquear o loop.
//  Como o driver usa tx_desc_auto_clear=true, parar de alimentar zera o DMA
//  (silencio) sozinho — keyOff() so aplica uma rampa curta p/ evitar clique.
// ===========================================================================
#include "audio.h"
#include "pins.h"
#include "es8311.h"
#include <Arduino.h>
#include <math.h>
#include "driver/i2s.h"

namespace audio {

static const int   SAMPLE_RATE = 16000;
static const int   MCLK_FREQ   = SAMPLE_RATE * 256;   // 4.096 MHz
static const int   CODEC_VOL   = 70;
static const float MASTER_GAIN = 0.30f;

static es8311_handle_t es = nullptr;
static bool            ready = false;

// Estado do sidetone (persiste entre chamadas de feedKey).
static float    keyPhase = 0.0f;
static uint16_t keyFreq  = 700;
static uint8_t  keyVol   = 80;

static bool initI2S() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 8,
        .dma_buf_len          = 256,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
        .fixed_mclk           = 0,
        .mclk_multiple        = I2S_MCLK_MULTIPLE_256,
        .bits_per_chan        = I2S_BITS_PER_CHAN_16BIT,
    };
    if (i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL) != ESP_OK) return false;

    i2s_pin_config_t pins = {
        .mck_io_num   = I2S_MCLK,
        .bck_io_num   = I2S_BCLK,
        .ws_io_num    = I2S_LRCK,
        .data_out_num = I2S_DOUT,
        .data_in_num  = I2S_PIN_NO_CHANGE,
    };
    if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) return false;
    i2s_zero_dma_buffer(I2S_NUM_0);
    return true;
}

static bool initCodec() {
    es = es8311_create(I2C_NUM_0, ES8311_ADDRRES_0);
    if (!es) return false;

    es8311_clock_config_t clk = {
        .mclk_inverted      = false,
        .sclk_inverted      = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency     = MCLK_FREQ,
        .sample_frequency   = SAMPLE_RATE,
    };
    if (es8311_init(es, &clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16) != ESP_OK)
        return false;
    es8311_sample_frequency_config(es, MCLK_FREQ, SAMPLE_RATE);
    es8311_voice_volume_set(es, CODEC_VOL, NULL);
    es8311_microphone_config(es, false);
    return true;
}

bool begin() {
    pinMode(PA_CTRL, OUTPUT);
    digitalWrite(PA_CTRL, LOW);

    if (!initI2S())   { ready = false; return false; }
    if (!initCodec()) { ready = false; return false; }

    digitalWrite(PA_CTRL, HIGH);
    ready = true;
    return true;
}

void tone(uint16_t freq, uint16_t ms, uint8_t vol) {
    if (!ready || freq == 0 || ms == 0) return;

    const int totalFrames = (int)((long)SAMPLE_RATE * ms / 1000);
    const float amp = (vol / 100.0f) * MASTER_GAIN * 32767.0f;
    const float dphase = 2.0f * (float)M_PI * freq / SAMPLE_RATE;
    const int attack  = SAMPLE_RATE * 4 / 1000;
    const int release = SAMPLE_RATE * 8 / 1000;

    static int16_t buf[256 * 2];
    float phase = 0.0f;
    int done = 0;
    while (done < totalFrames) {
        int n = totalFrames - done;
        if (n > 256) n = 256;
        for (int i = 0; i < n; i++) {
            int idx = done + i;
            float env = 1.0f;
            if (idx < attack)                     env = (float)idx / attack;
            else if (idx > totalFrames - release) env = (float)(totalFrames - idx) / release;

            int16_t s = (int16_t)(sinf(phase) * amp * env);
            phase += dphase;
            if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
            buf[2 * i]     = s;
            buf[2 * i + 1] = s;
        }
        size_t written = 0;
        i2s_write(I2S_NUM_0, buf, n * 2 * sizeof(int16_t), &written, portMAX_DELAY);
        done += n;
    }
}

// --- Sidetone -------------------------------------------------------------
void keyReset() { keyPhase = 0.0f; }

void feedKey(uint16_t freq, uint8_t vol) {
    if (!ready) return;
    keyFreq = freq; keyVol = vol;

    const int   N = 128;                              // ~8ms @ 16kHz
    const float amp = (vol / 100.0f) * MASTER_GAIN * 32767.0f;
    const float dphase = 2.0f * (float)M_PI * freq / SAMPLE_RATE;

    static int16_t buf[128 * 2];
    for (int i = 0; i < N; i++) {
        int16_t s = (int16_t)(sinf(keyPhase) * amp);
        keyPhase += dphase;
        if (keyPhase > 2.0f * (float)M_PI) keyPhase -= 2.0f * (float)M_PI;
        buf[2 * i]     = s;
        buf[2 * i + 1] = s;
    }
    size_t written = 0;
    i2s_write(I2S_NUM_0, buf, N * 2 * sizeof(int16_t), &written, portMAX_DELAY);
}

void keyOff() {
    if (!ready) return;
    // Rampa de saida (~8ms) do ultimo tom ate zero, para nao "clicar".
    const int   N = 128;
    const float amp = (keyVol / 100.0f) * MASTER_GAIN * 32767.0f;
    const float dphase = 2.0f * (float)M_PI * keyFreq / SAMPLE_RATE;

    static int16_t buf[128 * 2];
    for (int i = 0; i < N; i++) {
        float env = (float)(N - i) / N;               // 1 -> 0
        int16_t s = (int16_t)(sinf(keyPhase) * amp * env);
        keyPhase += dphase;
        if (keyPhase > 2.0f * (float)M_PI) keyPhase -= 2.0f * (float)M_PI;
        buf[2 * i]     = s;
        buf[2 * i + 1] = s;
    }
    size_t written = 0;
    i2s_write(I2S_NUM_0, buf, N * 2 * sizeof(int16_t), &written, portMAX_DELAY);
    keyPhase = 0.0f;
}

// --- Efeitos --------------------------------------------------------------
void sfxMenu()    { tone(880, 35, 70); }
void sfxGood()    { tone(784, 90, 80); tone(1046, 130, 82); }
void sfxBad()     { tone(300, 120, 75); tone(220, 200, 75); }
void sfxLevelUp() { tone(659, 90, 80); tone(831, 90, 80); tone(988, 90, 82); tone(1318, 220, 85); }
void sfxShutdown(){ tone(660, 80, 72); tone(520, 80, 72); tone(392, 80, 72); tone(262, 240, 72); }

} // namespace audio
