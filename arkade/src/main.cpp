// ===========================================================================
//  main.cpp — "Arcade" na SpotPear ESP32-S3-Touch-LCD-1.54
//
//  Home com carrossel: arraste ←/→ para trocar de jogo, toque para jogar.
//  Botão PLUS volta para a home; segurar PWR ~2s desliga.
// ===========================================================================
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "pins.h"
#include "display.h"
#include "touch.h"
#include "audio.h"
#include "qmi8658.h"
#include "battery.h"
#include "sys.h"

#include "game_ttt.h"
#include "game_simon.h"
#include "game_snake.h"
#include "game_whack.h"
#include "game_maze.h"

// --- Catálogo de jogos ----------------------------------------------------
struct GameEntry { const char* name; void (*run)(); };
static const GameEntry GAMES[] = {
    { "Jogo da Velha", ttt::run   },
    { "Simon",         simon::run },
    { "Snake",         snake::run },
    { "Reflexo",       whack::run },
    { "Labirinto",     maze::run  },
};
static const int NGAMES = sizeof(GAMES) / sizeof(GAMES[0]);

// Cor de destaque por jogo (resolvida em tempo de desenho).
static uint16_t gameColor(int i) {
    switch (i % 5) {
        case 0:  return display::COL_X;
        case 1:  return display::COL_O;
        case 2:  return display::COL_WIN;
        case 3:  return display::COL_DRAW;
        default: return display::COL_TEXT;
    }
}

static int sel = 0;
static uint32_t lastBat = 0;

static void drawHome() {
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    display::text("ARCADE", display::W / 2, 22, 2, display::COL_TEXT);

    if (NGAMES > 1) {
        display::text("<", 14, display::H / 2, 3, display::COL_GRID);
        display::text(">", display::W - 14, display::H / 2, 3, display::COL_GRID);
    }
    display::text(GAMES[sel].name, display::W / 2, display::H / 2 - 6, 2, gameColor(sel));
    display::text("toque para jogar", display::W / 2, display::H / 2 + 34, 1, display::COL_TEXT);
    if (NGAMES > 1)
        display::text("arraste p/ trocar", display::W / 2, display::H / 2 + 50, 1, display::COL_GRID);

    // Pontinhos indicadores da posição no carrossel.
    int cx0 = display::W / 2 - (NGAMES - 1) * 7;
    for (int i = 0; i < NGAMES; i++)
        g->fillCircle(cx0 + i * 14, display::H - 18, 3,
                      i == sel ? gameColor(sel) : display::COL_GRID);

    display::drawBattery(battery::percent(), battery::charging());
    lastBat = sys::now();
}

void setup() {
    sys::begin();                 // trava a energia (BAT_EN) + botões — PRIMEIRO
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== ARCADE — ESP32-S3-Touch-LCD-1.54 ===");
    Serial.printf("PSRAM: %u bytes livre\n", (unsigned)ESP.getFreePsram());

    display::begin();
    touch::begin();               // faz Wire.begin (I2C)
    Serial.printf("[audio] ES8311: %s\n", audio::begin() ? "OK" : "NAO");
    Serial.printf("[imu] QMI8658: %s\n", imu::begin() ? "OK" : "NAO");
    battery::begin();
    Serial.printf("[bat] %.2f V  %d%%\n", battery::voltage(), battery::percent());

    drawHome();
}

void loop() {
    sys::wantExit();              // trata o PWR (na home o PLUS não faz nada)

    if (sys::now() - lastBat > 5000) {
        lastBat = sys::now();
        display::drawBattery(battery::percent(), battery::charging());
    }

    // Detecção de arraste/toque baseada na SOLTURA do dedo:
    //  movimento horizontal grande = swipe (troca de jogo); toque curto = jogar.
    static bool tracking = false;
    static int  startX = 0, startY = 0, lastX = 0, lastY = 0;
    int tx, ty;
    bool down = sys::touching(&tx, &ty);

    if (down && !tracking) { tracking = true; startX = lastX = tx; startY = lastY = ty; }
    else if (down)         { lastX = tx; lastY = ty; }
    else if (!down && tracking) {
        tracking = false;
        int dx = lastX - startX, dy = lastY - startY;
        if (abs(dx) > 40 && abs(dx) > abs(dy)) {           // swipe horizontal
            sel = (dx < 0) ? (sel + 1) % NGAMES : (sel - 1 + NGAMES) % NGAMES;
            audio::sfxTap();
            drawHome();
        } else {                                            // toque curto = jogar
            audio::sfxMenu();
            GAMES[sel].run();                               // bloqueia até voltar
            drawHome();
        }
    }
    delay(8);
}
