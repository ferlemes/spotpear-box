// ===========================================================================
//  main.cpp — Morse Code Game na SpotPear ESP32-S3-Touch-LCD-1.54.
//
//  Menu inicial (toque para escolher): Learn / Letters / Words / Listen /
//  Settings. O botao de cima (PLUS) volta de qualquer modo para o menu;
//  segurar o PWR ~2s desliga.
// ===========================================================================
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "pins.h"
#include "display.h"
#include "touch.h"
#include "audio.h"
#include "sys.h"
#include "ui.h"
#include "i18n.h"
#include "settings.h"

#include "tutorial.h"
#include "game_letters.h"
#include "game_words.h"
#include "game_listen.h"

// --- Menu -----------------------------------------------------------------
static const int NHOME = 5;
static const ui::Rect HOME_BTN[NHOME] = {
    { 20,  66, 200, 30 },
    { 20, 102, 200, 30 },
    { 20, 138, 200, 30 },
    { 20, 174, 200, 30 },
    { 20, 210, 200, 30 },
};
static const i18n::Key HOME_KEY[NHOME] = {
    i18n::MENU_LEARN, i18n::MENU_LETTERS, i18n::MENU_WORDS,
    i18n::MENU_LISTEN, i18n::MENU_SETTINGS,
};

static uint16_t homeColor(int i) {
    switch (i) {
        case 0:  return display::COL_ACCENT;
        case 1:  return display::COL_DOT;
        case 2:  return display::COL_DASH;
        case 3:  return display::COL_GOOD;
        default: return display::COL_TEXT;
    }
}

static void drawHome() {
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    display::text(i18n::t(i18n::TITLE), display::W / 2, 26, 3, display::COL_ACCENT);
    display::text(i18n::langName(i18n::lang), display::W / 2, 50, 1, display::COL_DIM);
    for (int i = 0; i < NHOME; i++)
        ui::button(HOME_BTN[i], i18n::t(HOME_KEY[i]), homeColor(i), false);
}

// --- Ajustes: idioma + nivel + velocidade ---------------------------------
static const char* LANG_CODE[3] = { "EN", "PT", "ES" };
static const ui::Rect LANG_BTN[3] = {
    { 10, 38, 72, 32 }, { 84, 38, 72, 32 }, { 158, 38, 72, 32 },
};
static const ui::Rect LVL_BTN[3] = {
    { 10, 92, 72, 32 }, { 84, 92, 72, 32 }, { 158, 92, 72, 32 },
};
static const ui::Rect SPD_BTN[3] = {
    { 10, 146, 72, 32 }, { 84, 146, 72, 32 }, { 158, 146, 72, 32 },
};

static void drawSettings() {
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    display::text(i18n::t(i18n::MENU_SETTINGS), display::W / 2, 12, 2, display::COL_ACCENT);

    display::text(i18n::t(i18n::SET_TITLE), display::W / 2, 30, 1, display::COL_DIM);   // Idioma
    for (int i = 0; i < 3; i++)
        ui::button(LANG_BTN[i], LANG_CODE[i], display::COL_TEXT, i18n::lang == (i18n::Lang)i);

    display::text(i18n::t(i18n::LEVEL), display::W / 2, 84, 1, display::COL_DIM);        // Nivel
    for (int i = 0; i < 3; i++)
        ui::button(LVL_BTN[i], settings::levelName(i), display::COL_TEXT, settings::level == i);

    display::text(i18n::t(i18n::SPEED_TITLE), display::W / 2, 138, 1, display::COL_DIM); // Velocidade
    for (int i = 0; i < 3; i++)
        ui::button(SPD_BTN[i], settings::speedName(i), display::COL_TEXT, settings::speed == i);

    display::text(i18n::t(i18n::BACK_HINT), display::W / 2, 200, 1, display::COL_DIM);
}

static void settingsScreen() {
    drawSettings();
    while (true) {
        if (sys::wantExit()) return;                // botao de cima -> menu
        int x, y;
        if (sys::tapped(&x, &y)) {
            for (int i = 0; i < 3; i++) {
                if (ui::hit(LANG_BTN[i], x, y)) { i18n::setLang((i18n::Lang)i); audio::sfxMenu(); drawSettings(); }
                if (ui::hit(LVL_BTN[i],  x, y)) { settings::setLevel(i);        audio::sfxMenu(); drawSettings(); }
                if (ui::hit(SPD_BTN[i],  x, y)) { settings::setSpeed(i);        audio::sfxMenu(); drawSettings(); }
            }
        }
        delay(8);
    }
}

static void dispatch(int i) {
    audio::sfxMenu();
    switch (i) {
        case 0: tutorial::run(); break;
        case 1: letters::run();  break;
        case 2: words::run();    break;
        case 3: listen::run();   break;
        case 4: settingsScreen(); break;
    }
    drawHome();
}

void setup() {
    sys::begin();                 // trava a energia (BAT_EN) + botoes — PRIMEIRO
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== MORSE CODE GAME — ESP32-S3-Touch-LCD-1.54 ===");

    display::begin();
    touch::begin();               // faz Wire.begin (I2C)
    Serial.printf("[audio] ES8311: %s\n", audio::begin() ? "OK" : "NAO");

    drawHome();
}

void loop() {
    sys::wantExit();              // trata o PWR (no menu o PLUS nao faz nada)

    int x, y;
    if (sys::tapped(&x, &y)) {
        for (int i = 0; i < NHOME; i++) {
            if (ui::hit(HOME_BTN[i], x, y)) { dispatch(i); break; }
        }
    }
    delay(8);
}
