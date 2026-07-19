// ===========================================================================
//  sys.cpp — Energia (trava BAT_EN, desliga real), botão voltar (PLUS), toque.
// ===========================================================================
#include "sys.h"
#include "pins.h"
#include "display.h"
#include "audio.h"
#include "touch.h"
#include <Arduino.h>
#include "esp_sleep.h"
#include "driver/rtc_io.h"

namespace sys {

static bool prevTouch = false;
static bool prevBack  = false;

void begin() {
    // ANTES DE TUDO: trava a energia da bateria ligada (senão, na bateria, só
    // fica ligado enquanto o PWR estiver pressionado).
    pinMode(BAT_EN, OUTPUT);
    digitalWrite(BAT_EN, HIGH);

    pinMode(BTN_PWR,   INPUT_PULLUP);
    pinMode(BTN_PLUS,  INPUT_PULLUP);   // "+" = tecla do Morse (morseKeyDown)
    pinMode(BTN_MINUS, INPUT_PULLUP);   // "-" = voltar ao menu (wantExit)
}

// Desliga de verdade: corta a energia da bateria (BAT_EN=LOW). No USB os 5V
// seguem, então cai em deep sleep como alternativa (acorda ao apertar o PWR).
static void powerOff() {
    Serial.println("[power] desligando (BAT_EN=LOW)");
    display::drawSleepScreen();
    audio::sfxShutdown();
    digitalWrite(PA_CTRL, LOW);
    delay(150);
    digitalWrite(BAT_EN, LOW);        // na bateria, o aparelho desliga AQUI

    delay(1200);                      // só chega aqui no USB
    digitalWrite(LCD_BL, LOW);
    while (digitalRead(BTN_PWR) == LOW) delay(10);
    rtc_gpio_pullup_en((gpio_num_t)BTN_PWR);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_PWR, 0);
    esp_deep_sleep_start();
}

static void checkPower() {
    static bool     armed = false;
    static uint32_t pressStart = 0;
    static bool     wasDown = false;
    bool down = (digitalRead(BTN_PWR) == LOW);
    if (!armed) { if (!down) armed = true; return; }   // espera soltar após ligar
    uint32_t n = millis();
    if (down && !wasDown) pressStart = n;
    if (down && (n - pressStart >= 2000)) powerOff();
    wasDown = down;
}

// "Voltar" = borda de descida do botao "-" (KEY_MINUS / GPIO0).
bool wantExit() {
    checkPower();
    bool p = (digitalRead(BTN_MINUS) == LOW);
    bool edge = p && !prevBack;
    prevBack = p;
    return edge;
}

// Tecla do Morse = botao "+" (KEY_PLUS / GPIO4) pressionado agora (LOW).
bool morseKeyDown() { return digitalRead(BTN_PLUS) == LOW; }

bool tapped(int* x, int* y) {
    uint16_t tx = 0, ty = 0;
    bool down = touch::read(&tx, &ty);
    bool t = down && !prevTouch;
    prevTouch = down;
    if (t) { if (x) *x = tx; if (y) *y = ty; }
    return t;
}

bool touching(int* x, int* y) {
    uint16_t tx = 0, ty = 0;
    bool down = touch::read(&tx, &ty);
    if (down) { if (x) *x = tx; if (y) *y = ty; }
    return down;
}

uint32_t now() { return millis(); }

} // namespace sys
