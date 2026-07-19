// ===========================================================================
//  game_letters.cpp — Digitar letras. O nivel de dificuldade define o TETO de
//  letras (settings::letterCap); dentro dele, o pool comeca nas vogais e
//  cresce conforme o jogador acerta (introducao gradual estilo Koch).
// ===========================================================================
#include "game_letters.h"
#include "challenge.h"
#include "display.h"
#include "ui.h"
#include "morse.h"
#include "audio.h"
#include "i18n.h"
#include "sys.h"
#include "settings.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <esp_random.h>

namespace letters {

static const int START_POOL = 5;   // 5 vogais (comeco de morse::ORDER)
static const int GROW_EVERY = 4;   // acertos p/ liberar mais letras
static const int GROW_STEP  = 2;   // letras liberadas por vez
static const int GLYPH_CY   = 140; // faixa dos glifos digitados

// Estado passado ao callback de redesenho.
struct View { char target; int pool; int cap; int score; };

static void redraw(void* u) {
    View* v = (View*)u;
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);

    display::text(i18n::t(i18n::MENU_LETTERS), display::W / 2, 14, 2, display::COL_ACCENT);

    // Nivel (Beg/Int/Adv) + progresso do pool dentro do teto + placar.
    char buf[28];
    snprintf(buf, sizeof buf, "%s  %d/%d   %s %d",
             settings::levelName(settings::level), v->pool, v->cap,
             i18n::t(i18n::SCORE), v->score);
    display::text(buf, display::W / 2, 34, 1, display::COL_TEXT);
    display::text(i18n::t(i18n::BACK_HINT), display::W / 2, 50, 1, display::COL_DIM);

    char letter[2] = { v->target, 0 };
    display::text(letter, display::W / 2, 92, 6, display::COL_TEXT);

    ui::drawKey(false);
}

void run() {
    View v;
    v.score = 0;
    v.cap   = settings::letterCap();
    v.pool  = (START_POOL < v.cap) ? START_POOL : v.cap;
    int sinceGrow = 0;

    while (true) {
        // Sorteia uma letra do pool atual (ORDER[0..pool)).
        v.target = morse::ORDER[esp_random() % v.pool];

        challenge::Ctx c;
        c.target  = v.target;
        c.glyphCx = display::W / 2;
        c.glyphCy = GLYPH_CY;
        c.redraw  = redraw;
        c.user    = &v;

        int r = challenge::run(c);
        if (r == 0) return;                 // saiu

        // Acertou.
        v.score++;
        display::screen()->fillRect(0, GLYPH_CY - 18, display::W, 36, display::COL_BG);
        display::text(i18n::t(i18n::CORRECT), display::W / 2, GLYPH_CY, 2, display::COL_GOOD);
        delay(650);

        // Libera mais letras (ate o teto do nivel).
        if (++sinceGrow >= GROW_EVERY && v.pool < v.cap) {
            sinceGrow = 0;
            v.pool += GROW_STEP;
            if (v.pool > v.cap) v.pool = v.cap;
            audio::sfxLevelUp();
        }
    }
}

} // namespace letters
