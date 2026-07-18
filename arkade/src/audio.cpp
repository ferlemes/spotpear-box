// ===========================================================================
//  audio.cpp — Som pelo ES8311.
//
//  Fluxo: o I2S (mestre) gera MCLK/BCLK/WS e envia as amostras; o ES8311
//  (escravo, configurado por I2C) converte em analógico; o NS4150B (habilitado
//  por PA_CTRL) amplifica pro speaker. Config: 16 kHz, 16 bits, MCLK=256*fs.
//
//  Usa a API de I2S LEGADA (driver/i2s.h) porque o core instalado é o 2.0.17
//  (IDF 4.4). No core 3.x seria a API nova (i2s_std.h).
// ===========================================================================
#include "audio.h"
#include "pins.h"
#include "es8311.h"
#include <Arduino.h>
#include <math.h>
#include "driver/i2s.h"

namespace audio {

static const int      SAMPLE_RATE = 16000;
static const int      MCLK_FREQ   = SAMPLE_RATE * 256;   // 4.096 MHz (está na tabela)

// --- Volume (dois estágios) ----------------------------------------------
//  CODEC_VOL   = volume do DAC do ES8311 (0..100).
//  MASTER_GAIN = fração da escala cheia da onda (headroom). O speaker é
//                pequeno: valores altos saturam o amplificador (distorção).
//  Se ficar baixo demais, suba os dois; se distorcer, baixe o MASTER_GAIN.
static const int      CODEC_VOL   = 70;
static const float    MASTER_GAIN = 0.30f;

static es8311_handle_t es = nullptr;
static bool            ready = false;

static bool initI2S() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,   // estéreo (L=R)
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 8,
        .dma_buf_len          = 256,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,                         // zera buffer no fim (sem ruído)
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
    // Wire já foi iniciado por touch::begin() (mesmo barramento I2C 42/41).
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
    digitalWrite(PA_CTRL, LOW);          // amplificador desligado durante o setup

    if (!initI2S())   { ready = false; return false; }
    if (!initCodec()) { ready = false; return false; }

    digitalWrite(PA_CTRL, HIGH);         // liga o amplificador do speaker
    ready = true;
    return true;
}

void tone(uint16_t freq, uint16_t ms, uint8_t vol) {
    if (!ready || freq == 0 || ms == 0) return;

    const int totalFrames = (int)((long)SAMPLE_RATE * ms / 1000);
    const float amp = (vol / 100.0f) * MASTER_GAIN * 32767.0f;
    const float dphase = 2.0f * (float)M_PI * freq / SAMPLE_RATE;
    const int attack  = SAMPLE_RATE * 4 / 1000;              // rampa de 4 ms
    const int release = SAMPLE_RATE * 8 / 1000;              // rampa de 8 ms

    static int16_t buf[256 * 2];   // 256 frames estéreo
    float phase = 0.0f;
    int done = 0;
    while (done < totalFrames) {
        int n = totalFrames - done;
        if (n > 256) n = 256;
        for (int i = 0; i < n; i++) {
            int idx = done + i;
            float env = 1.0f;                                // envelope anti-clique
            if (idx < attack)                 env = (float)idx / attack;
            else if (idx > totalFrames - release) env = (float)(totalFrames - idx) / release;

            int16_t s = (int16_t)(sinf(phase) * amp * env);
            phase += dphase;
            if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
            buf[2 * i]     = s;   // L
            buf[2 * i + 1] = s;   // R
        }
        size_t written = 0;
        i2s_write(I2S_NUM_0, buf, n * 2 * sizeof(int16_t), &written, portMAX_DELAY);
        done += n;
    }
}

void sfxMenu() { tone(1046, 45, 70); }
void sfxTap()  { tone(880, 35, 70); }

// Fanfarra de vitória (E - G# - B - E agudo, com a última nota sustentada).
void sfxWin() {
    tone(659, 90, 80); tone(830, 90, 80); tone(988, 90, 82);
    tone(1318, 120, 85); tone(988, 70, 78); tone(1318, 280, 85);
}
// Derrota: "womp womp" descendo.
void sfxLose() {
    tone(440, 130, 78); tone(392, 130, 78); tone(330, 150, 78); tone(247, 320, 78);
}
void sfxDraw() { tone(587, 100, 75); tone(523, 100, 75); tone(587, 180, 75); }
void sfxShake(){ tone(700, 45, 70); tone(400, 70, 70); }   // "reset"
// Desligar: descida "power-down".
void sfxShutdown(){ tone(660, 80, 72); tone(520, 80, 72); tone(392, 80, 72); tone(262, 240, 72); }

} // namespace audio
