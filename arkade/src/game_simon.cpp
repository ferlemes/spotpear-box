// ===========================================================================
//  game_simon.cpp — Simon / Genius (jogo de memoria com 4 pads coloridos).
//
//  A cada rodada, a sequencia ganha um pad aleatorio. O jogo mostra a
//  sequencia inteira (pad acende + tom) e o jogador precisa repeti-la na
//  ordem. Acertou tudo -> proxima rodada. Errou -> fim de jogo.
// ===========================================================================
#include "game_simon.h"
#include "display.h"
#include "audio.h"
#include "sys.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <esp_random.h>

namespace simon {

// --- Geometria ------------------------------------------------------------
static const int TOP    = 30;    // faixa de status no topo (rodada / recorde)
static const int MARGIN = 6;     // margem externa dos pads
static const int GAP    = 6;     // espaco entre pads
static const int PW     = 111;   // largura do pad = (240 - 2*MARGIN - GAP)/2
static const int PH     = 96;    // altura  do pad = (210 - 2*MARGIN - GAP)/2
static const int RAD    = 14;    // raio das bordas arredondadas

// --- Definicao dos 4 pads (vermelho, verde, azul, amarelo) ----------------
static const uint8_t  PR[4]   = { 230,  70,  80, 235 };  // componente R
static const uint8_t  PG[4]   = {  70, 200, 150, 205 };  // componente G
static const uint8_t  PB[4]   = {  70, 100, 240,  80 };  // componente B
static const uint16_t PTONE[4] = { 330, 415, 494, 659 }; // tom de cada pad (Hz)

// --- Estado da partida ----------------------------------------------------
static const int MAXLEN = 32;
static uint8_t   seq[MAXLEN];    // pads sorteados (0..3)
static int       len;            // tamanho atual da sequencia = rodada
static int       best = 0;       // recorde (persiste entre partidas/entradas)

// --- Cores dos pads (calculadas a partir do RGB base) ---------------------
static uint16_t dimCol(int i) {   // apagado: ~35% do brilho
    Arduino_GFX* g = display::screen();
    return g->color565(PR[i] * 35 / 100, PG[i] * 35 / 100, PB[i] * 35 / 100);
}
static uint16_t litCol(int i) {   // aceso: base misturada com branco (flash)
    Arduino_GFX* g = display::screen();
    return g->color565(PR[i] + (255 - PR[i]) / 2,
                       PG[i] + (255 - PG[i]) / 2,
                       PB[i] + (255 - PB[i]) / 2);
}

// Canto superior esquerdo de cada pad no layout 2x2.
static int padX(int i) { return (i % 2 == 0) ? MARGIN : (MARGIN + PW + GAP); }
static int padY(int i) { return (i < 2) ? (TOP + MARGIN) : (TOP + MARGIN + PH + GAP); }

// Mapeia um toque para o pad correspondente (0..3) ou -1 se fora dos pads.
static int hitPad(int x, int y) {
    for (int i = 0; i < 4; i++) {
        int px = padX(i), py = padY(i);
        if (x >= px && x < px + PW && y >= py && y < py + PH) return i;
    }
    return -1;
}

// --- Desenho --------------------------------------------------------------

// Desenha um pad apagado ou aceso (com borda branca simulando o flash).
static void drawPad(int i, bool lit) {
    Arduino_GFX* g = display::screen();
    int x = padX(i), y = padY(i);
    if (lit) {
        g->fillRoundRect(x, y, PW, PH, RAD, litCol(i));
        uint16_t white = g->color565(255, 255, 255);
        g->drawRoundRect(x,     y,     PW,     PH,     RAD, white);
        g->drawRoundRect(x + 1, y + 1, PW - 2, PH - 2, RAD, white);
    } else {
        g->fillRoundRect(x, y, PW, PH, RAD, dimCol(i));
    }
}

// Faixa fina de dica entre o topo e os pads (avisa de quem e a vez).
static void setHint(uint16_t color) {
    display::screen()->fillRect(0, TOP, display::W, MARGIN, color);
}

// Cabecalho: rodada atual (esquerda) e recorde (direita).
static void drawHeader() {
    Arduino_GFX* g = display::screen();
    g->fillRect(0, 0, display::W, TOP, display::COL_BG);
    char buf[16];
    snprintf(buf, sizeof buf, "Rodada %d", len);
    display::text(buf, 62, TOP / 2, 2, display::COL_TEXT);
    snprintf(buf, sizeof buf, "Rec %d", best);
    display::text(buf, 196, TOP / 2, 2, display::COL_DRAW);
}

// Redesenha a tela inteira (fundo + pads apagados + cabecalho).
static void drawBoard() {
    display::screen()->fillScreen(display::COL_BG);
    for (int i = 0; i < 4; i++) drawPad(i, false);
    drawHeader();
}

// Acende um pad, toca seu tom (bloqueante) e volta ao estado apagado.
static void flashPad(int i, uint16_t ms) {
    drawPad(i, true);
    audio::tone(PTONE[i], ms, 80);
    drawPad(i, false);
}

// --- Fluxo do jogo --------------------------------------------------------

static void addPad() {
    if (len < MAXLEN) seq[len++] = (uint8_t)(esp_random() % 4);
}

static void newGame() {
    len = 0;
    addPad();            // rodada 1 ja comeca com um pad
    drawBoard();
}

// Reproduz a sequencia inteira. Retorna true se o jogador pediu p/ sair.
static bool playSequence() {
    setHint(display::COL_DRAW);   // amarelo = "preste atencao"
    delay(500);
    for (int i = 0; i < len; i++) {
        if (sys::wantExit()) return true;
        flashPad(seq[i], 350);
        delay(180);               // pausa com o pad apagado entre acendimentos
    }
    return false;
}

// Tela de fim de jogo (errou). Pisca vermelho e mostra a rodada alcancada.
static void gameOver() {
    audio::sfxLose();
    Arduino_GFX* g = display::screen();
    g->fillScreen(g->color565(200, 40, 40));   // flash vermelho
    delay(180);
    g->fillScreen(display::COL_BG);
    display::text("Errou!", display::W / 2, 66, 3, display::COL_X);
    char buf[16];
    snprintf(buf, sizeof buf, "Rodada %d", len);
    display::text(buf, display::W / 2, 118, 2, display::COL_TEXT);
    snprintf(buf, sizeof buf, "Recorde %d", best);
    display::text(buf, display::W / 2, 148, 2, display::COL_DRAW);
    display::text("toque p/ jogar", display::W / 2, 200, 2, display::COL_TEXT);
}

// Tela de vitoria (raro: completou a sequencia maxima).
static void winGame() {
    audio::sfxWin();
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    display::text("Venceu!", display::W / 2, 80, 3, display::COL_WIN);
    display::text("Memoria de aco!", display::W / 2, 130, 2, display::COL_TEXT);
    display::text("toque p/ jogar", display::W / 2, 200, 2, display::COL_TEXT);
}

void run() {
    newGame();
    enum St { SHOW, GUESS, OVER } st = SHOW;   // GUESS: evita macro Arduino INPUT
    int inputPos = 0;

    while (true) {
        if (sys::wantExit()) return;      // botao PLUS -> volta pra home

        int x, y;
        bool tap = sys::tapped(&x, &y);   // sempre poll (mantem a borda em dia)

        switch (st) {
            case SHOW:
                drawHeader();
                if (playSequence()) return;   // abortado pelo PLUS
                setHint(display::COL_WIN);     // verde = "sua vez"
                inputPos = 0;
                st = GUESS;
                break;

            case GUESS:
                if (tap) {
                    int p = hitPad(x, y);
                    if (p >= 0) {
                        flashPad(p, 250);      // feedback do toque
                        if (p == seq[inputPos]) {
                            // acertou este passo
                            if (++inputPos >= len) {
                                // rodada completa!
                                if (len > best) best = len;
                                audio::tone(784, 120, 80);   // "ding" de acerto
                                if (len >= MAXLEN) { winGame(); st = OVER; break; }
                                delay(350);
                                addPad();
                                drawHeader();
                                st = SHOW;
                            }
                        } else {
                            gameOver();
                            st = OVER;
                        }
                    }
                }
                break;

            case OVER:
                if (tap) { newGame(); st = SHOW; }
                break;
        }
        delay(8);
    }
}

} // namespace simon
