// ===========================================================================
//  game_ttt.cpp — Jogo da Velha (display + touch + IA minimax + som + IMU).
// ===========================================================================
#include "game_ttt.h"
#include "game.h"
#include "display.h"
#include "audio.h"
#include "qmi8658.h"
#include "sys.h"
#include <Arduino.h>

namespace ttt {

static Game       game;
static int        mode = 0;             // 0=2 jog., 1=fácil, 2=média, 3=difícil
static Difficulty diff = AI_HARD;
static int        scoreX, scoreO, scoreDraws;
static const Cell AI_PLAYER = O;        // humano = X, IA = O

static void showTurn() {
    if (game.turn == X) display::drawStatus("Vez do X", display::COL_X);
    else                display::drawStatus("Vez do O", display::COL_O);
}

static void startGame(int m) {
    mode = m;
    diff = (m == 1) ? AI_EASY : (m == 2) ? AI_MEDIUM : AI_HARD;
    game.reset();
    audio::sfxMenu();
    display::drawBoardFrame();
    showTurn();
}

static void finishGame() {
    bool aiMode = (mode != 0);
    const char* headline;
    uint16_t hcol;
    if (game.result == WIN_X || game.result == WIN_O) {
        display::drawWinLine(game.winLine);
        bool xWon = (game.result == WIN_X);
        if (xWon) scoreX++; else scoreO++;
        hcol = xWon ? display::COL_X : display::COL_O;
        if (aiMode) headline = xWon ? "VOCE GANHOU" : "IA GANHOU";
        else        headline = xWon ? "X VENCEU"    : "O VENCEU";
        bool happy = (!aiMode) || xWon;
        if (happy) audio::sfxWin(); else audio::sfxLose();
    } else {
        scoreDraws++;
        headline = "EMPATE";
        hcol = display::COL_DRAW;
        audio::sfxDraw();
    }
    delay(900);
    display::drawResultScreen(headline, hcol, scoreX, scoreO, scoreDraws, aiMode);
}

static void applyMove(int idx) {
    Cell who = game.turn;
    if (!game.place(idx)) return;
    display::animateMark(idx, who);
    if (game.result == ONGOING) { audio::sfxTap(); showTurn(); }
    else                        finishGame();
}

void run() {
    scoreX = scoreO = scoreDraws = 0;
    enum St { MENU, PLAY, OVER } st = MENU;
    display::drawMenu();

    while (true) {
        if (sys::wantExit()) return;        // botão PLUS -> volta pra home

        // Chacoalhar reinicia a partida (mesmo modo).
        float peak;
        if (imu::shaken(&peak) && (st == PLAY || st == OVER)) {
            audio::sfxShake();
            game.reset();
            st = PLAY;
            display::drawBoardFrame();
            showTurn();
        }

        int x, y;
        bool tap = sys::tapped(&x, &y);     // sempre poll (mantém a borda em dia)

        switch (st) {
            case MENU:
                if (tap) { int m = display::menuHit(x, y); if (m >= 0) { startGame(m); st = PLAY; } }
                break;
            case PLAY:
                if (mode != 0 && game.turn == AI_PLAYER) {
                    display::drawStatus("IA pensando...", display::COL_O);
                    delay(450);
                    int mv = aiChooseMove(game.board, AI_PLAYER, diff);
                    if (mv >= 0) applyMove(mv);
                    if (game.result != ONGOING) st = OVER;
                } else if (tap) {
                    int c = display::cellFromTouch(x, y);
                    if (c >= 0) { applyMove(c); if (game.result != ONGOING) st = OVER; }
                }
                break;
            case OVER:
                if (tap) { st = MENU; display::drawMenu(); }
                break;
        }
        delay(6);
    }
}

} // namespace ttt
