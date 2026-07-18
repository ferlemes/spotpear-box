// ===========================================================================
//  game_whack.cpp — Reflexo (Whack-a-Mole): acerte a célula acesa em 30s.
// ===========================================================================
#include "game_whack.h"
#include "display.h"
#include "audio.h"
#include "sys.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <esp_random.h>

namespace whack {

// --- Geometria da grade 3x3 -----------------------------------------------
static const int MARGIN = 8;
static const int TOP    = 30;                       // faixa superior (placar+tempo)
static const int GAP    = 8;                        // espaço entre células
static const int CELL   = 62;                       // lado da célula
static const int GRID   = 3 * CELL + 2 * GAP;       // = 202
static const int ORIGX  = (display::W - GRID) / 2;  // centraliza na horizontal
static const int ORIGY  = TOP;

static const uint32_t ROUND_MS = 30000;             // duração da rodada

// --- Estado ----------------------------------------------------------------
static int      score;        // pontos da rodada atual
static int      lit;          // célula acesa (0..8), -1 se nenhuma
static uint32_t roundStart;   // início da rodada
static uint32_t moleAt;       // quando a toupeira atual acendeu
static int      lastSec;      // último segundo desenhado (evita flicker)
static int      best = 0;     // melhor pontuação (persiste na sessão)

static uint16_t COL_MOLE;     // cor da toupeira (resolvida em run)
static uint16_t COL_HOLE;     // cor do "buraco" das células apagadas

// canto superior-esquerdo de uma célula
static int cellX(int idx) { return ORIGX + (idx % 3) * (CELL + GAP); }
static int cellY(int idx) { return ORIGY + (idx / 3) * (CELL + GAP); }

// desenha uma célula acesa (alvo) ou apagada (buraco)
static void drawCell(int idx, bool on) {
    Arduino_GFX* g = display::screen();
    int x = cellX(idx), y = cellY(idx);
    if (on) {
        g->fillRoundRect(x, y, CELL, CELL, 12, COL_MOLE);
        int cx = x + CELL / 2, cy = y + CELL / 2;    // alvo concêntrico
        g->fillCircle(cx, cy, CELL / 4, display::COL_BG);
        g->fillCircle(cx, cy, CELL / 7, COL_MOLE);
    } else {
        g->fillRoundRect(x, y, CELL, CELL, 12, display::COL_GRID);
        g->fillRoundRect(x + 6, y + 6, CELL - 12, CELL - 12, 8, COL_HOLE);
    }
}

// toque (x,y) -> índice de célula 0..8, ou -1 se fora da grade
static int cellFromTap(int x, int y) {
    if (x < ORIGX || y < ORIGY) return -1;
    int col = (x - ORIGX) / (CELL + GAP);
    int row = (y - ORIGY) / (CELL + GAP);
    if (col > 2 || row > 2) return -1;
    return row * 3 + col;
}

// placar à esquerda (redesenha só quando muda -> sem flicker)
static void drawScore() {
    Arduino_GFX* g = display::screen();
    g->fillRect(0, 0, 150, TOP - 2, display::COL_BG);
    char buf[24];
    snprintf(buf, sizeof(buf), "Pontos: %d", score);
    display::text(buf, 66, 14, 2, display::COL_TEXT);
}

// tempo restante à direita (redesenha só quando o segundo muda)
static void drawTime(int sec) {
    Arduino_GFX* g = display::screen();
    g->fillRect(150, 0, display::W - 150, TOP - 2, display::COL_BG);
    char buf[12];
    snprintf(buf, sizeof(buf), "%ds", sec);
    uint16_t c = (sec <= 5) ? display::COL_X : display::COL_TEXT;
    display::text(buf, 196, 14, 2, c);
}

// acende uma nova toupeira, sempre diferente da atual
static void relocate() {
    int old = lit;
    int n;
    do { n = esp_random() % 9; } while (n == old);
    if (old >= 0) drawCell(old, false);
    lit = n;
    drawCell(lit, true);
    moleAt = sys::now();
}

// (re)inicia uma rodada de 30s
static void newRound() {
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    score   = 0;
    lit     = -1;
    lastSec = -1;
    for (int i = 0; i < 9; i++) drawCell(i, false);
    drawScore();
    roundStart = sys::now();
    relocate();
}

// tela de fim de rodada
static void showResult() {
    if (score > best) best = score;
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    display::text("TEMPO!", display::W / 2, 50, 3, COL_MOLE);
    char buf[24];
    snprintf(buf, sizeof(buf), "Pontos: %d", score);
    display::text(buf, display::W / 2, 108, 2, display::COL_TEXT);
    snprintf(buf, sizeof(buf), "Recorde: %d", best);
    display::text(buf, display::W / 2, 138, 2, display::COL_DRAW);
    display::text("toque p/ jogar de novo", display::W / 2, 194, 1, display::COL_TEXT);
    display::text("PLUS: sair", display::W / 2, 214, 1, display::COL_GRID);
    audio::sfxWin();
}

void run() {
    Arduino_GFX* g = display::screen();
    COL_MOLE = g->color565(240, 200, 70);   // amarelo brilhante (toupeira)
    COL_HOLE = g->color565(40, 44, 60);      // buraco escuro
    newRound();

    while (true) {
        if (sys::wantExit()) return;         // botão PLUS -> volta pra home
        uint32_t t = sys::now();

        // --- fim da rodada ---
        if (t - roundStart >= ROUND_MS) {
            showResult();
            // espera toque p/ recomeçar (PLUS sai)
            while (true) {
                if (sys::wantExit()) return;
                int x, y;
                if (sys::tapped(&x, &y)) break;
                delay(10);
            }
            newRound();
            continue;
        }

        // --- tempo restante (atualiza só quando o segundo inteiro muda) ---
        int sec = (int)((ROUND_MS - (t - roundStart) + 999) / 1000);   // arredonda p/ cima
        if (sec != lastSec) { lastSec = sec; drawTime(sec); }

        // --- toupeira foge sozinha se demorar demais (encurta com o placar) ---
        uint32_t tout = (score * 20 >= 600) ? 400 : (1000 - (uint32_t)score * 20);
        if (t - moleAt >= tout) relocate();

        // --- entrada ---
        int x, y;
        if (sys::tapped(&x, &y)) {
            int c = cellFromTap(x, y);
            if (c == lit) {                  // acertou!
                score++;
                audio::tone(880, 40, 80);
                drawScore();
                relocate();
            } else if (c >= 0) {             // errou uma célula apagada
                audio::tone(200, 60, 60);
            }
        }
        delay(10);
    }
}

} // namespace whack
