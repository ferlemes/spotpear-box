// ===========================================================================
//  tutorial.cpp — Modo "Learn": como jogar + treino guiado letra a letra.
//
//  Para cada letra (vogais primeiro): mostra a letra, toca o codigo e deixa o
//  codigo a mostra como referencia; entao o jogador DIGITA a letra no botao
//  fisico "+" (mesma logica do challenge). Acertou -> proxima letra.
// ===========================================================================
#include "tutorial.h"
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

namespace tutorial {

static const int GLYPH_CY = 140;   // faixa dos glifos digitados

// Tela inicial "como jogar". Retorna false se o jogador pediu p/ sair.
static bool howToPlay() {
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    display::text(i18n::t(i18n::MENU_LEARN), display::W / 2, 20, 2, display::COL_ACCENT);

    // Exemplos visuais de ponto e traco + o que faz cada botao.
    ui::drawMorse(".", 46, 70, display::COL_DOT, display::COL_DASH);
    ui::drawMorse("-", 46, 100, display::COL_DOT, display::COL_DASH);
    display::text(i18n::t(i18n::HOWTO_1), display::W / 2 + 22, 70, 1, display::COL_TEXT);
    display::text(i18n::t(i18n::HOWTO_2), display::W / 2 + 22, 100, 1, display::COL_TEXT);
    display::text(i18n::t(i18n::HOWTO_3), display::W / 2, 136, 1, display::COL_TEXT);
    display::text(i18n::t(i18n::BACK_HINT), display::W / 2, 158, 1, display::COL_DIM);
    display::text(i18n::t(i18n::TAP_CONTINUE), display::W / 2, 205, 2, display::COL_GOOD);

    while (true) {
        if (sys::wantExit()) return false;
        int x, y;
        if (sys::tapped(&x, &y)) { audio::sfxMenu(); return true; }
        delay(8);
    }
}

// Estado passado ao callback de redesenho.
struct State { int idx; };

// Tela-base de uma letra: letra grande + codigo de referencia + indicador KEY.
static void redrawLetter(void* u) {
    State* s = (State*)u;
    char c = morse::ORDER[s->idx];

    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);

    display::text(i18n::t(i18n::MENU_LEARN), display::W / 2, 14, 2, display::COL_ACCENT);
    char buf[8];
    snprintf(buf, sizeof buf, "%d/%d", s->idx + 1, settings::letterCap());
    display::text(buf, display::W / 2, 34, 1, display::COL_DIM);

    char letter[2] = { c, 0 };
    display::text(letter, display::W / 2, 74, 5, display::COL_TEXT);

    // Codigo da letra em tom apagado = "e assim que se escreve" (referencia).
    ui::drawMorse(morse::code(c), display::W / 2, 108, display::COL_DIM, display::COL_DIM);

    ui::drawKey(false);
}

void run() {
    if (!howToPlay()) return;

    State st; st.idx = 0;
    while (true) {
        char c = morse::ORDER[st.idx];
        redrawLetter(&st);
        morse::playLetter(c);           // ouve o codigo com a letra na tela

        challenge::Ctx cx;
        cx.target  = c;
        cx.glyphCx = display::W / 2;
        cx.glyphCy = GLYPH_CY;
        cx.redraw  = redrawLetter;
        cx.user    = &st;

        int r = challenge::run(cx);     // digita a letra no botao "+"
        if (r == 0) return;             // "-" -> menu

        // Acertou -> proxima letra.
        display::screen()->fillRect(0, GLYPH_CY - 18, display::W, 36, display::COL_BG);
        display::text(i18n::t(i18n::CORRECT), display::W / 2, GLYPH_CY, 2, display::COL_GOOD);
        delay(600);

        st.idx++;
        if (st.idx >= settings::letterCap()) { audio::sfxLevelUp(); return; }
    }
}

} // namespace tutorial
