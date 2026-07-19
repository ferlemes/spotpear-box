// ===========================================================================
//  game_listen.cpp — Reverso: ouvir/ver o codigo e escolher a palavra.
// ===========================================================================
#include "game_listen.h"
#include "display.h"
#include "ui.h"
#include "morse.h"
#include "audio.h"
#include "i18n.h"
#include "sys.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <string.h>
#include <esp_random.h>

namespace listen {

static const int NOPT = 4;
static const ui::Rect REPLAY_BTN = { 70, 58, 100, 26 };
static const ui::Rect OPT[NOPT] = {
    { 20,  94, 200, 30 },
    { 20, 130, 200, 30 },
    { 20, 166, 200, 30 },
    { 20, 202, 200, 30 },
};

// Monta o codigo da palavra como texto (letras separadas por 2 espacos).
static void wordCodeText(const char* word, char* out, int cap) {
    out[0] = 0;
    for (int i = 0; word[i]; i++) {
        strncat(out, morse::code(word[i]), cap - strlen(out) - 1);
        if (word[i + 1]) strncat(out, "  ", cap - strlen(out) - 1);
    }
}

static void drawScreen(const char* opts[], const char* target) {
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);

    display::text(i18n::t(i18n::MENU_LISTEN), display::W / 2, 12, 2, display::COL_ACCENT);
    display::text(i18n::t(i18n::LISTEN_PROMPT), display::W / 2, 32, 1, display::COL_TEXT);

    char codebuf[40];
    wordCodeText(target, codebuf, sizeof codebuf);
    display::text(codebuf, display::W / 2, 46, 1, display::COL_DASH);

    ui::button(REPLAY_BTN, i18n::t(i18n::REPLAY), display::COL_ACCENT, false);
    for (int i = 0; i < NOPT; i++)
        ui::button(OPT[i], opts[i], display::COL_TEXT, false);
}

// Sorteia a rodada: escolhe o alvo e 3 distratores distintos, e embaralha.
// So usa palavras validas p/ o nivel atual (fallback: lista inteira).
static void newRound(const char** opts, int* correctIdx, const char** target) {
    int pool[32];
    int n = i18n::levelWords(pool, 32);
    if (n < NOPT) {                          // poucas do nivel: usa a lista toda
        n = i18n::wordCount();
        for (int i = 0; i < n; i++) pool[i] = i;
    }

    int t = pool[esp_random() % n];
    *target = i18n::wordAt(t);

    // indices escolhidos (comeca com o alvo).
    int chosen[NOPT]; chosen[0] = t; int have = 1;
    while (have < NOPT) {
        int cand = pool[esp_random() % n];
        bool dup = false;
        for (int i = 0; i < have; i++) if (chosen[i] == cand) { dup = true; break; }
        // evita tambem palavras iguais em texto (listas podem repetir entre si)
        if (!dup) {
            for (int i = 0; i < have; i++)
                if (strcmp(i18n::wordAt(chosen[i]), i18n::wordAt(cand)) == 0) { dup = true; break; }
        }
        if (!dup) chosen[have++] = cand;
    }

    // Fisher-Yates.
    for (int i = NOPT - 1; i > 0; i--) {
        int j = esp_random() % (i + 1);
        int tmp = chosen[i]; chosen[i] = chosen[j]; chosen[j] = tmp;
    }

    for (int i = 0; i < NOPT; i++) {
        opts[i] = i18n::wordAt(chosen[i]);
        if (chosen[i] == t) *correctIdx = i;
    }
}

void run() {
    const char* opts[NOPT];
    const char* target;
    int correctIdx;

    newRound(opts, &correctIdx, &target);
    drawScreen(opts, target);
    delay(300);
    morse::playWord(target);

    while (true) {
        if (sys::wantExit()) return;

        int x, y;
        if (sys::tapped(&x, &y)) {
            if (ui::hit(REPLAY_BTN, x, y)) {
                morse::playWord(target);
                continue;
            }
            for (int i = 0; i < NOPT; i++) {
                if (!ui::hit(OPT[i], x, y)) continue;
                if (i == correctIdx) {
                    ui::button(OPT[i], opts[i], display::COL_GOOD, true);
                    audio::sfxLevelUp();
                    delay(900);
                    newRound(opts, &correctIdx, &target);
                    drawScreen(opts, target);
                    delay(300);
                    morse::playWord(target);
                } else {
                    ui::button(OPT[i], opts[i], display::COL_BAD, true);
                    audio::sfxBad();
                    delay(500);
                    ui::button(OPT[i], opts[i], display::COL_TEXT, false);   // volta ao normal
                }
                break;
            }
        }
        delay(8);
    }
}

} // namespace listen
