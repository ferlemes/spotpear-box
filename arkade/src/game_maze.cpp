// ===========================================================================
//  game_maze.cpp — Labirinto Inclinado: role a bola até a saída inclinando a
//  caixa. Movimento em passos de grade (confiável), lido do acelerômetro.
// ===========================================================================
#include "game_maze.h"
#include "display.h"
#include "audio.h"
#include "qmi8658.h"
#include "sys.h"
#include <Arduino_GFX_Library.h>
#include <Arduino.h>
#include <math.h>

namespace maze {

// --- Geometria: 10x10 células de 24px preenchem os 240x240 da tela ----------
static const int MAZE_W = 10, MAZE_H = 10;
static const int CELL   = 24;              // 10 * 24 == 240

// Célula inicial (canto sup. esq.) e a saída (canto inf. dir.).
static const int START_X = 1, START_Y = 1;
static const int GOAL_X  = 8, GOAL_Y  = 8;

// Passo de tempo entre leituras de inclinação (ms) e limiar de inclinação (g).
static const uint32_t STEP_MS  = 130;
static const float    TILT_TH  = 0.30f;

// Ajuste de orientação: mapeiam o sinal da inclinação -> direção na grade.
// Se a bola andar para o lado errado, troque o respectivo valor para -1.
static const int INV_X = 1;   // eixo X do IMU  -> coluna
static const int INV_Y = 1;   // eixo Y do IMU  -> linha

// Labirinto: 1 = parede, 0 = livre. MAZE[linha][coluna] (y, x).
// Serpentina com caminho livre contínuo de (1,1) até (8,8) — verificado à mão.
static const uint8_t MAZE[MAZE_H][MAZE_W] = {
    { 1,1,1,1,1,1,1,1,1,1 },
    { 1,0,0,0,0,0,0,0,0,1 },   // corredor superior (início em col 1)
    { 1,1,1,1,1,1,1,1,0,1 },   // desce pela col 8
    { 1,0,0,0,0,0,0,0,0,1 },   // corredor
    { 1,0,1,1,1,1,1,1,1,1 },   // desce pela col 1
    { 1,0,0,0,0,0,0,0,0,1 },   // corredor
    { 1,1,1,1,1,1,1,1,0,1 },   // desce pela col 8
    { 1,0,0,0,0,0,0,0,0,1 },   // corredor
    { 1,0,0,0,0,0,0,0,0,1 },   // saída em (8,8)
    { 1,1,1,1,1,1,1,1,1,1 },
};

// --- Estado da bola ---------------------------------------------------------
static int      ballX, ballY;
static int      steps;
static uint32_t startMs, lastStep;

static inline bool isWall(int cx, int cy) {
    if (cx < 0 || cx >= MAZE_W || cy < 0 || cy >= MAZE_H) return true;
    return MAZE[cy][cx] != 0;
}

// Cor de fundo de uma célula (parede azul, saída verde, resto = fundo).
static uint16_t cellColor(Arduino_GFX* g, int cx, int cy) {
    if (isWall(cx, cy))                        return g->color565(60, 80, 140);
    if (cx == GOAL_X && cy == GOAL_Y)          return g->color565(70, 200, 110);
    return display::COL_BG;
}

// Pinta uma célula (usado no desenho inicial e para apagar a bola antiga).
static void drawCell(Arduino_GFX* g, int cx, int cy) {
    g->fillRect(cx * CELL, cy * CELL, CELL, CELL, cellColor(g, cx, cy));
}

// Desenha a bola centralizada na sua célula.
static void drawBall(Arduino_GFX* g) {
    g->fillCircle(ballX * CELL + CELL / 2, ballY * CELL + CELL / 2,
                  CELL / 2 - 4, g->color565(240, 200, 80));
}

// Desenha o labirinto inteiro + a bola (uma vez, ao (re)iniciar).
static void drawMaze(Arduino_GFX* g) {
    g->fillScreen(display::COL_BG);
    for (int cy = 0; cy < MAZE_H; cy++)
        for (int cx = 0; cx < MAZE_W; cx++)
            if (isWall(cx, cy) || (cx == GOAL_X && cy == GOAL_Y))
                drawCell(g, cx, cy);
    drawBall(g);
}

static void resetGame(Arduino_GFX* g) {
    ballX = START_X; ballY = START_Y;
    steps = 0;
    startMs = sys::now();
    lastStep = startMs;
    drawMaze(g);
}

// Lê a inclinação e dá um passo na grade se o alvo estiver livre.
static void stepTick(Arduino_GFX* g) {
    float ax, ay, az;
    if (!imu::readAccelG(&ax, &ay, &az)) return;

    int dx = 0, dy = 0;
    // Escolhe o eixo dominante para um passo por vez.
    if (fabsf(ax) > fabsf(ay) && fabsf(ax) > TILT_TH)
        dx = (ax > 0 ? 1 : -1) * INV_X;
    else if (fabsf(ay) > TILT_TH)
        dy = (ay > 0 ? 1 : -1) * INV_Y;

    if (dx == 0 && dy == 0) return;

    int nx = ballX + dx, ny = ballY + dy;
    if (isWall(nx, ny)) return;             // parede: não move

    int ox = ballX, oy = ballY;
    ballX = nx; ballY = ny; steps++;
    drawCell(g, ox, oy);                    // apaga a bola antiga
    drawBall(g);                            // desenha na nova célula
    audio::tone(700, 15, 20);               // "tique" suave
}

static void showWin() {
    uint32_t secs = (sys::now() - startMs) / 1000;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d passos  %us", steps, (unsigned)secs);

    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    display::text("VOCE",        120,  60, 3, display::COL_WIN);
    display::text("CHEGOU!",     120,  95, 3, display::COL_WIN);
    display::text(buf,           120, 140, 2, display::COL_TEXT);
    display::text("toque p/ jogar de novo", 120, 175, 1, display::COL_TEXT);
    audio::sfxWin();
}

void run() {
    Arduino_GFX* g = display::screen();
    enum St { PLAY, OVER } st = PLAY;
    resetGame(g);

    while (true) {
        if (sys::wantExit()) return;        // botão PLUS -> volta pra home

        uint32_t t = sys::now();
        int x, y;
        bool tap = sys::tapped(&x, &y);     // mantém a borda do toque em dia

        switch (st) {
            case PLAY:
                if (t - lastStep >= STEP_MS) {
                    lastStep = t;
                    stepTick(g);
                    if (ballX == GOAL_X && ballY == GOAL_Y) {
                        showWin();
                        st = OVER;
                    }
                }
                break;
            case OVER:
                if (tap) { resetGame(g); st = PLAY; }
                break;
        }
        delay(15);
    }
}

} // namespace maze
