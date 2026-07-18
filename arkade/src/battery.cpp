// ===========================================================================
//  battery.cpp — BAT_ADC = VBAT dividido por 3 (R27=200K + R32=100K).
//    VBAT = leitura_no_pino * 3.  ADC calibrado via analogReadMilliVolts().
// ===========================================================================
#include "battery.h"
#include "pins.h"
#include <Arduino.h>

namespace battery {

static const float DIVIDER = 3.0f;   // (200K + 100K) / 100K
static const float V_FULL  = 4.20f;  // ~100%
static const float V_EMPTY = 3.30f;  // ~0%

void begin() {
    analogReadResolution(12);
    analogSetPinAttenuation(BAT_ADC, ADC_11db);   // faixa ~0..3.1V
    pinMode(CHG_STAT, INPUT_PULLUP);
}

float voltage() {
    const int N = 16;
    uint32_t sum = 0;
    for (int i = 0; i < N; i++) sum += analogReadMilliVolts(BAT_ADC);
    float pin_mV = sum / (float)N;
    return (pin_mV * DIVIDER) / 1000.0f;          // volts na bateria
}

int percent() {
    float p = (voltage() - V_EMPTY) / (V_FULL - V_EMPTY) * 100.0f;
    if (p < 0)   p = 0;
    if (p > 100) p = 100;
    return (int)(p + 0.5f);
}

bool charging() {
    // CHG_STAT do carregador é dreno-aberto: baixo = carregando.
    return digitalRead(CHG_STAT) == LOW;
}

} // namespace battery
