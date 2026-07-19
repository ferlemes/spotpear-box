// ===========================================================================
//  ui.h — Desenhos reutilizaveis: retangulos/botoes, glifos de morse
//  (pontos e tracos) e o grande botao "KEY" (a tecla telegrafica na tela).
// ===========================================================================
#pragma once
#include <stdint.h>

namespace ui {
    struct Rect { int x, y, w, h; };

    bool hit(const Rect& r, int x, int y);

    // Botao arredondado. filled=true pinta o fundo (rotulo na cor do fundo da
    // tela); filled=false desenha so a borda (rotulo na cor 'color').
    void button(const Rect& r, const char* label, uint16_t color, bool filled);

    // Desenha um padrao morse (string de '.' e '-') centralizado em (cx,cy).
    void drawMorse(const char* pat, int cx, int cy, uint16_t dotCol, uint16_t dashCol);

    // Como drawMorse, mas com um elemento "em curso" (prov: '.' ou '-', ou 0)
    // desenhado por ultimo na cor liveCol — feedback ao vivo enquanto segura.
    void drawMorseLive(const char* committed, char prov, int cx, int cy,
                       uint16_t dotCol, uint16_t dashCol, uint16_t liveCol);

    // Botao KEY fixo na parte de baixo da tela (a "tecla"). pressed muda a cor.
    extern const Rect KEY_RECT;
    void drawKey(bool pressed);
}
