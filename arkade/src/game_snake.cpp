// ===========================================================================
//  game_snake.cpp — Snake controlado por inclinação (acelerômetro QMI8658).
//  Incline a placa para virar; coma a comida vermelha e cresça sem bater.
// ===========================================================================
#include "game_snake.h"
#include "display.h"
#include "audio.h"
#include "qmi8658.h"
#include "sys.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <esp_random.h>
#include <math.h>

namespace snake {

// --- Geometria do tabuleiro -----------------------------------------------
static const int CELL      = 16;            // lado de cada célula (px)
static const int GRID_COLS = 15;            // 15 * 16 = 240 (largura cheia)
static const int GRID_ROWS = 14;            // 14 * 16 = 224
static const int ORIGIN_Y  = 16;            // topo reservado ao placar
static const int MAX_LEN   = 100;           // comprimento máximo da cobra
static const uint32_t TICK_MS = 170;        // passo de movimento

// Inversão de eixos: dependendo da orientação da placa, o mapeamento
// inclinação->direção pode ficar espelhado. Troque para -1 p/ inverter.
const int INV_X = 1, INV_Y = 1;
static const float TILT_TH = 0.25f;         // limiar mínimo de inclinação (g)

// --- Cores ----------------------------------------------------------------
static uint16_t COL_BODY, COL_HEAD, COL_FOOD;

// --- Estado ---------------------------------------------------------------
struct Pt { int8_t cx, cy; };
static Pt   body[MAX_LEN];                  // body[0] = cabeça
static int  length;
static int  dx, dy;                         // direção atual
static int  pdx, pdy;                        // direção pendente (aplicada no tick)
static Pt   food;
static int  score;
static uint32_t lastTick;
static int  best = 0;                        // recorde da sessão (persiste)

// Sorteia uma célula vazia para a comida.
static void placeFood() {
    while (true) {
        int8_t fx = esp_random() % GRID_COLS;
        int8_t fy = esp_random() % GRID_ROWS;
        bool onSnake = false;
        for (int i = 0; i < length; i++)
            if (body[i].cx == fx && body[i].cy == fy) { onSnake = true; break; }
        if (!onSnake) { food.cx = fx; food.cy = fy; return; }
    }
}

// Reinicia a partida (mantém o recorde).
static void resetGame() {
    length = 3;
    int8_t sx = GRID_COLS / 2, sy = GRID_ROWS / 2;   // ~centro (7,7)
    for (int i = 0; i < length; i++) { body[i].cx = sx - i; body[i].cy = sy; }
    dx = pdx = 1; dy = pdy = 0;                       // começa indo p/ direita
    score = 0;
    placeFood();
    lastTick = sys::now();
}

// Desenha uma célula do grid como quadradinho arredondado.
static void drawCell(Arduino_GFX* g, int cx, int cy, uint16_t col) {
    int px = cx * CELL;
    int py = ORIGIN_Y + cy * CELL;
    g->fillRoundRect(px + 1, py + 1, CELL - 2, CELL - 2, 3, col);
}

// Redesenha a tela inteira (barato neste tamanho).
static void render() {
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    char buf[40];
    snprintf(buf, sizeof(buf), "Pontos: %d   Rec: %d", score, best);
    display::text(buf, display::W / 2, 8, 1, display::COL_TEXT);
    drawCell(g, food.cx, food.cy, COL_FOOD);         // comida
    for (int i = length - 1; i >= 0; i--)            // cauda -> cabeça
        drawCell(g, body[i].cx, body[i].cy, (i == 0) ? COL_HEAD : COL_BODY);
}

// Lê a inclinação e atualiza a direção pendente (sem permitir giro de 180°).
static void readTilt() {
    float ax, ay, az;
    if (!imu::readAccelG(&ax, &ay, &az)) return;
    float tx = ax * INV_X;                            // eixo -> direção
    float ty = ay * INV_Y;
    int ndx = 0, ndy = 0;
    if (fabsf(tx) > fabsf(ty) && fabsf(tx) > TILT_TH)      ndx = (tx > 0) ? 1 : -1;
    else if (fabsf(ty) > TILT_TH)                          ndy = (ty > 0) ? 1 : -1;
    if (ndx == 0 && ndy == 0) return;                // inclinação insuficiente
    if (ndx == -dx && ndy == -dy) return;            // proíbe reversão direta
    pdx = ndx; pdy = ndy;
}

// Avança um passo. Retorna false se a cobra morreu (parede ou corpo).
static bool step() {
    dx = pdx; dy = pdy;
    int nx = body[0].cx + dx;
    int ny = body[0].cy + dy;
    if (nx < 0 || nx >= GRID_COLS || ny < 0 || ny >= GRID_ROWS) return false;
    bool grow = (nx == food.cx && ny == food.cy);
    // Sem crescer, a cauda libera sua célula, então a ignoramos na colisão.
    int checkLen = grow ? length : length - 1;
    for (int i = 0; i < checkLen; i++)
        if (body[i].cx == nx && body[i].cy == ny) return false;
    if (grow && length < MAX_LEN) length++;
    for (int i = length - 1; i > 0; i--) body[i] = body[i - 1];   // desliza corpo
    body[0].cx = nx; body[0].cy = ny;
    if (grow) { score++; audio::tone(880, 40, 80); placeFood(); }
    return true;
}

// Tela de fim de jogo: placar, recorde e dica para reiniciar.
static void gameOver() {
    if (score > best) best = score;
    audio::sfxLose();
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    char buf[24];
    display::text("GAME OVER", display::W / 2, 70, 3, display::COL_X);
    snprintf(buf, sizeof(buf), "Pontos: %d", score);
    display::text(buf, display::W / 2, 120, 2, display::COL_TEXT);
    snprintf(buf, sizeof(buf), "Recorde: %d", best);
    display::text(buf, display::W / 2, 150, 2, display::COL_WIN);
    display::text("toque p/ jogar", display::W / 2, 195, 2, display::COL_TEXT);
}

void run() {
    Arduino_GFX* g = display::screen();
    COL_BODY = g->color565( 80, 210, 110);           // verde do corpo
    COL_HEAD = g->color565(140, 240, 160);           // cabeça mais clara
    COL_FOOD = g->color565(230,  60,  60);           // comida vermelha

    resetGame();
    render();
    enum St { PLAY, OVER } st = PLAY;

    while (true) {
        if (sys::wantExit()) return;                 // PLUS -> volta pra home

        int tx, ty;
        bool tap = sys::tapped(&tx, &ty);            // poll (mantém a borda em dia)

        if (st == PLAY) {
            readTilt();                              // atualiza direção pendente
            uint32_t t = sys::now();
            if (t - lastTick >= TICK_MS) {
                lastTick = t;
                if (step()) render();
                else       { gameOver(); st = OVER; }
            }
        } else {                                     // OVER
            if (tap) { resetGame(); render(); st = PLAY; }
        }
        delay(15);
    }
}

} // namespace snake
