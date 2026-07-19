// ===========================================================================
//  ui.cpp — Implementacao dos desenhos reutilizaveis.
// ===========================================================================
#include "ui.h"
#include "display.h"
#include "i18n.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

namespace ui {

// Metricas dos glifos de morse.
static const int DOT_D  = 12;   // diametro do ponto
static const int DASH_W = 26;   // largura do traco
static const int GLYPH_H = 12;  // altura comum
static const int GAP    = 8;    // espaco entre elementos

const Rect KEY_RECT = { 16, 165, 208, 66 };

bool hit(const Rect& r, int x, int y) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

void button(const Rect& r, const char* label, uint16_t color, bool filled) {
    Arduino_GFX* g = display::screen();
    if (filled) {
        g->fillRoundRect(r.x, r.y, r.w, r.h, 10, color);
        display::text(label, r.x + r.w / 2, r.y + r.h / 2, 2, display::COL_BG);
    } else {
        g->drawRoundRect(r.x,     r.y,     r.w,     r.h,     10, color);
        g->drawRoundRect(r.x + 1, r.y + 1, r.w - 2, r.h - 2, 10, color);
        display::text(label, r.x + r.w / 2, r.y + r.h / 2, 2, color);
    }
}

static int patternWidth(const char* pat) {
    int n = 0, w = 0;
    for (int i = 0; pat[i]; i++) {
        w += (pat[i] == '.') ? DOT_D : DASH_W;
        n++;
    }
    if (n > 1) w += GAP * (n - 1);
    return w;
}

void drawMorse(const char* pat, int cx, int cy, uint16_t dotCol, uint16_t dashCol) {
    if (!pat || !pat[0]) return;
    Arduino_GFX* g = display::screen();
    int x = cx - patternWidth(pat) / 2;
    for (int i = 0; pat[i]; i++) {
        if (pat[i] == '.') {
            g->fillCircle(x + DOT_D / 2, cy, DOT_D / 2, dotCol);
            x += DOT_D + GAP;
        } else {
            g->fillRoundRect(x, cy - GLYPH_H / 2, DASH_W, GLYPH_H, GLYPH_H / 2, dashCol);
            x += DASH_W + GAP;
        }
    }
}

void drawMorseLive(const char* committed, char prov, int cx, int cy,
                   uint16_t dotCol, uint16_t dashCol, uint16_t liveCol) {
    // Monta 'committed' + o elemento em curso (prov) para centralizar tudo junto.
    char full[12]; int n = 0;
    for (; committed && committed[n] && n < 10; n++) full[n] = committed[n];
    if (prov) full[n++] = prov;
    full[n] = 0;
    if (n == 0) return;

    Arduino_GFX* g = display::screen();
    int x = cx - patternWidth(full) / 2;
    for (int i = 0; i < n; i++) {
        bool isProv = (prov && i == n - 1);              // o ultimo e o "em curso"
        if (full[i] == '.') {
            g->fillCircle(x + DOT_D / 2, cy, DOT_D / 2, isProv ? liveCol : dotCol);
            x += DOT_D + GAP;
        } else {
            g->fillRoundRect(x, cy - GLYPH_H / 2, DASH_W, GLYPH_H, GLYPH_H / 2,
                             isProv ? liveCol : dashCol);
            x += DASH_W + GAP;
        }
    }
}

// Indicador da tecla (NAO e tocavel): acende quando o botao FISICO "+"
// (KEY_PLUS) esta pressionado. A entrada do morse e sempre pelo botao fisico.
void drawKey(bool pressed) {
    Arduino_GFX* g = display::screen();
    uint16_t bg   = pressed ? display::COL_ACCENT : display::COL_KEY;
    uint16_t txt  = pressed ? display::COL_BG      : display::COL_TEXT;
    g->fillRoundRect(KEY_RECT.x, KEY_RECT.y, KEY_RECT.w, KEY_RECT.h, 12, bg);
    g->drawRoundRect(KEY_RECT.x, KEY_RECT.y, KEY_RECT.w, KEY_RECT.h, 12, display::COL_ACCENT);
    display::text("KEY  [+]", KEY_RECT.x + KEY_RECT.w / 2,
                  KEY_RECT.y + 22, 3, txt);
    display::text("short = .    long = -", KEY_RECT.x + KEY_RECT.w / 2,
                  KEY_RECT.y + KEY_RECT.h - 14, 1, txt);
}

} // namespace ui
