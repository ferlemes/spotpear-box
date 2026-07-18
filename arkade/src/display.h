// ===========================================================================
//  display.h — Renderização no LCD ST7789 240x240 via Arduino_GFX.
// ===========================================================================
#pragma once
#include <stdint.h>
#include "game.h"

class Arduino_GFX;   // forward decl (jogos incluem <Arduino_GFX_Library.h>)

namespace display {
    constexpr int W = 240, H = 240;

    void begin();

    // --- Helpers genéricos (usados por qualquer jogo) ---
    Arduino_GFX* screen();   // objeto de desenho para primitivas livres
    void text(const char* s, int cx, int cy, uint8_t size, uint16_t color); // centralizado

    // --- Menu inicial ---
    void drawMenu();
    // Retorna 0=2 jogadores, 1=IA fácil, 2=IA difícil, ou -1 se fora dos botões.
    int  menuHit(uint16_t x, uint16_t y);

    // --- Partida ---
    void drawBoardFrame();                 // limpa tela e desenha grade vazia
    void drawMark(int idx, Cell who);      // desenha X ou O numa célula
    void animateMark(int idx, Cell who);   // desenha com "pop" (cresce até o tamanho)
    void drawStatus(const char* text, uint16_t color);
    void drawFooter(const char* text, uint16_t color);   // dica no rodapé
    void drawWinLine(int8_t line);         // risca a trinca vencedora
    int  cellFromTouch(uint16_t x, uint16_t y);  // toque -> índice 0..8 (ou -1)

    // Tela final com placar da sessão. aiMode muda os rótulos (Voce/IA vs X/O).
    void drawResultScreen(const char* headline, uint16_t hcol,
                          int sx, int so, int draws, bool aiMode);

    void drawSleepScreen();   // "ate mais!" antes de dormir
    void drawSplash(const char* title, const char* sub);  // splash de diagnóstico
    void drawBattery(int percent, bool charging);         // ícone no canto sup. direito

    // Cores prontas para as mensagens de status.
    extern uint16_t COL_BG, COL_GRID, COL_X, COL_O, COL_TEXT, COL_WIN, COL_DRAW;
}
