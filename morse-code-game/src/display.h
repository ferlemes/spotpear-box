// ===========================================================================
//  display.h — Renderizacao no LCD ST7789 240x240 via Arduino_GFX.
//
//  Versao enxuta (so o que o Morse Game usa): inicializacao, objeto de
//  desenho, texto centralizado/alinhado e a paleta de cores.
// ===========================================================================
#pragma once
#include <stdint.h>

class Arduino_GFX;   // forward decl (os .cpp incluem <Arduino_GFX_Library.h>)

namespace display {
    constexpr int W = 240, H = 240;

    void begin();

    Arduino_GFX* screen();   // objeto de desenho para primitivas livres

    // Texto na fonte classica 6x8 (largura = 6*size, altura = 8*size).
    void text(const char* s, int cx, int cy, uint8_t size, uint16_t color);      // centralizado
    void textLeft(const char* s, int x, int y, uint8_t size, uint16_t color);    // alinhado a esq.

    void drawSleepScreen();   // "ate mais!" antes de dormir (usado por sys.cpp)

    // Paleta (RGB565). DOT/DASH sao as cores do ponto e do traco.
    extern uint16_t COL_BG, COL_ACCENT, COL_TEXT, COL_DIM,
                    COL_DOT, COL_DASH, COL_GOOD, COL_BAD, COL_KEY;
}
