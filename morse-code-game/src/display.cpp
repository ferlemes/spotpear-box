// ===========================================================================
//  display.cpp — Parte grafica (init do ST7789, texto e paleta).
// ===========================================================================
#include "display.h"
#include "pins.h"
#include "i18n.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

namespace display {

static Arduino_DataBus* bus = nullptr;
static Arduino_GFX*     gfx = nullptr;

uint16_t COL_BG, COL_ACCENT, COL_TEXT, COL_DIM,
         COL_DOT, COL_DASH, COL_GOOD, COL_BAD, COL_KEY;

Arduino_GFX* screen() { return gfx; }

void text(const char* s, int cx, int cy, uint8_t size, uint16_t color) {
    int w = (int)strlen(s) * 6 * size;
    int h = 8 * size;
    gfx->setTextColor(color);
    gfx->setTextSize(size);
    gfx->setCursor(cx - w / 2, cy - h / 2);
    gfx->print(s);
}

void textLeft(const char* s, int x, int y, uint8_t size, uint16_t color) {
    gfx->setTextColor(color);
    gfx->setTextSize(size);
    gfx->setCursor(x, y);
    gfx->print(s);
}

void begin() {
    bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI, LCD_MISO);
    gfx = new Arduino_ST7789(bus, LCD_RST, 0 /*rotacao*/, true /*IPS*/,
                             LCD_W, LCD_H);
    gfx->begin();

    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);          // backlight ligado

    // Paleta (RGB565).
    COL_BG     = gfx->color565(16, 18, 27);     // fundo escuro
    COL_ACCENT = gfx->color565(70, 200, 235);   // ciano (titulos/realces)
    COL_TEXT   = gfx->color565(235, 238, 245);  // texto claro
    COL_DIM    = gfx->color565(90, 98, 120);    // texto/apagado discreto
    COL_DOT    = gfx->color565(120, 230, 140);  // ponto = verde
    COL_DASH   = gfx->color565(255, 170, 70);   // traco = laranja
    COL_GOOD   = gfx->color565(120, 230, 140);  // acerto
    COL_BAD    = gfx->color565(255, 95, 85);    // erro
    COL_KEY    = gfx->color565(45, 52, 74);     // botao KEY em repouso

    gfx->fillScreen(COL_BG);
}

void drawSleepScreen() {
    gfx->fillScreen(COL_BG);
    text(i18n::t(i18n::SLEEP_BYE),  W / 2, H / 2 - 14, 3, COL_ACCENT);
    text(i18n::t(i18n::SLEEP_HINT), W / 2, H / 2 + 26, 1, COL_TEXT);
}

} // namespace display
