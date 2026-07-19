// ===========================================================================
//  game_words.cpp — Jogo de palavras.
//
//  Ao entrar, um seletor define duas coisas (independentes):
//    - Tamanho:     3-4 letras (curtas, filtradas pelo nivel) ou 5-6 (longas).
//    - Conferencia: por LETRA (confere/ajuda a cada letra) ou por PALAVRA
//                   (voce digita a palavra toda e so no fim confere).
// ===========================================================================
#include "game_words.h"
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
#include <string.h>
#include <esp_random.h>

namespace words {

static const uint16_t SIDETONE_FREQ = 700;
static const uint32_t RELEASE_DB_MS = 30;
static const int GLYPH_CY = 132;   // faixa dos glifos (modo por-letra)
static const int WW_CY    = 134;   // faixa dos glifos (modo palavra-inteira)

// Escolhas do seletor (persistem enquanto o aparelho fica ligado).
static bool s_long  = false;   // false = 3-4 letras; true = 5-6 letras
static bool s_whole = false;   // false = confere por letra; true = palavra toda

// --------------------------------------------------------------------------
//  Comuns
// --------------------------------------------------------------------------

// Sorteia uma palavra conforme o tamanho escolhido. Curtas respeitam o nivel;
// longas usam o alfabeto todo (sao o "nivel 2", mais avancado).
static const char* pickWord() {
    if (s_long)
        return i18n::wordLongAt(esp_random() % i18n::wordLongCount());
    int idxs[32];
    int n = i18n::levelWords(idxs, 32);
    if (n <= 0) return i18n::wordAt(esp_random() % i18n::wordCount());
    return i18n::wordAt(idxs[esp_random() % n]);
}

// Desenha a palavra. idx<0 = sem realce (todas neutras); idx>=0 = feita/atual/falta.
static void drawWordRow(const char* word, int idx, int y) {
    int n = strlen(word);
    int sz = (n <= 4) ? 4 : 3;               // encolhe p/ palavras longas caberem
    int cw = 6 * sz;
    int gap = (sz == 4) ? 8 : 6;
    int total = n * cw + (n - 1) * gap;
    int x = display::W / 2 - total / 2;

    Arduino_GFX* g = display::screen();
    for (int i = 0; i < n; i++) {
        uint16_t col = (idx < 0)   ? display::COL_TEXT :
                       (i < idx)   ? display::COL_GOOD :
                       (i == idx)  ? display::COL_ACCENT : display::COL_DIM;
        char ch[2] = { word[i], 0 };
        display::textLeft(ch, x, y, sz, col);
        if (idx >= 0 && i == idx)
            g->fillRect(x, y + 8 * sz + 3, cw, 3, display::COL_ACCENT);
        x += cw + gap;
    }
}

static void clearBand(int cy) {
    display::screen()->fillRect(0, cy - 17, display::W, 34, display::COL_BG);
}

// --------------------------------------------------------------------------
//  Modo por-letra (reaproveita challenge::run, com dica a cada letra)
// --------------------------------------------------------------------------
struct View { const char* word; int idx; };

static void redrawPerLetter(void* u) {
    View* v = (View*)u;
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    display::text(i18n::t(i18n::MENU_WORDS), display::W / 2, 14, 2, display::COL_ACCENT);
    display::text(i18n::t(i18n::BACK_HINT), display::W / 2, 36, 1, display::COL_DIM);
    drawWordRow(v->word, v->idx, 66);
    ui::drawKey(false);
}

static void playPerLetter() {
    View v; v.idx = 0;
    v.word = pickWord();

    while (true) {
        challenge::Ctx c;
        c.target  = v.word[v.idx];
        c.glyphCx = display::W / 2;
        c.glyphCy = GLYPH_CY;
        c.redraw  = redrawPerLetter;
        c.user    = &v;

        int r = challenge::run(c);
        if (r == 0) return;                 // saiu

        v.idx++;
        if (v.word[v.idx] == 0) {
            audio::sfxLevelUp();
            redrawPerLetter(&v);
            display::text(i18n::t(i18n::WORD_DONE), display::W / 2, GLYPH_CY, 2, display::COL_GOOD);
            delay(1200);
            v.idx = 0;
            v.word = pickWord();
        }
    }
}

// --------------------------------------------------------------------------
//  Modo palavra-inteira (sem dica; digita tudo, confere no fim)
// --------------------------------------------------------------------------

static void drawWholeBase(const char* word, const char* decoded) {
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    display::text(i18n::t(i18n::MENU_WORDS), display::W / 2, 14, 2, display::COL_ACCENT);
    drawWordRow(word, -1, 44);                            // palavra a soletrar
    if (decoded && decoded[0])                            // o que ja foi digitado
        display::text(decoded, display::W / 2, 85, 3, display::COL_ACCENT);
    display::text(i18n::t(i18n::SPELL_HINT), display::W / 2, 112, 1, display::COL_DIM);
    ui::drawKey(false);
}

static void drawTyped(const char* decoded) {
    display::screen()->fillRect(0, 70, display::W, 32, display::COL_BG);
    if (decoded && decoded[0])
        display::text(decoded, display::W / 2, 85, 3, display::COL_ACCENT);
}

// Joga UMA palavra inteira. Retorna 1 = acertou; 0 = saiu (botao "-").
static int playOneWholeWord(const char* word) {
    int wordLen = strlen(word);
    char decoded[12]; int dlen = 0; decoded[0] = 0;
    char entered[8];  int elen = 0; entered[0] = 0;

    bool     keyDown = false, pendingUp = false;
    uint32_t downStart = 0, upAt = 0, lastAct = sys::now();
    char     lastProv = 0;

    drawWholeBase(word, decoded);

    while (true) {
        if (sys::wantExit()) return 0;

        uint32_t now = sys::now();
        bool onKey = sys::morseKeyDown();

        if (onKey) {
            pendingUp = false;
            if (!keyDown) {
                keyDown = true; downStart = now; lastProv = 0;
                audio::keyReset(); ui::drawKey(true);
            }
            audio::feedKey(SIDETONE_FREQ, 80);
            lastAct = now;
            char prov = (now - downStart < settings::dotMaxMs()) ? '.' : '-';
            if (prov != lastProv) {
                clearBand(WW_CY);
                ui::drawMorseLive(entered, prov, display::W / 2, WW_CY,
                                  display::COL_DOT, display::COL_DASH, display::COL_ACCENT);
                lastProv = prov;
            }
        } else if (keyDown) {
            if (!pendingUp) { pendingUp = true; upAt = now; }
            if (now - upAt >= RELEASE_DB_MS) {
                keyDown = false; pendingUp = false;
                audio::keyOff();
                uint32_t held = upAt - downStart;
                if (elen < (int)sizeof(entered) - 1) {
                    entered[elen++] = (held < settings::dotMaxMs()) ? '.' : '-';
                    entered[elen] = 0;
                }
                ui::drawKey(false);
                clearBand(WW_CY);
                ui::drawMorse(entered, display::W / 2, WW_CY, display::COL_DOT, display::COL_DASH);
                lastAct = now;
            } else {
                audio::feedKey(SIDETONE_FREQ, 80);
            }
        } else {
            delay(6);
        }

        // Fim de UMA letra: acumula a letra decodificada e limpa p/ a proxima.
        if (!keyDown && elen > 0 && (now - lastAct) > settings::letterGapMs()) {
            if (dlen < (int)sizeof(decoded) - 1) {
                decoded[dlen++] = morse::decode(entered);
                decoded[dlen] = 0;
            }
            entered[0] = 0; elen = 0; lastProv = 0;
            clearBand(WW_CY);
            drawTyped(decoded);

            if (dlen >= wordLen) {
                // Palavra completa -> confere tudo de uma vez.
                if (strcmp(decoded, word) == 0) {
                    audio::sfxLevelUp();
                    clearBand(WW_CY);
                    display::text(i18n::t(i18n::WORD_DONE), display::W / 2, WW_CY, 2, display::COL_GOOD);
                    delay(1200);
                    return 1;
                }
                // Errou: mostra a palavra certa (vermelho) e TOCA o codigo dela.
                audio::sfxBad();
                drawTyped("");
                display::text(word, display::W / 2, 85, 3, display::COL_BAD);
                display::text(i18n::t(i18n::TRY_AGAIN), display::W / 2, WW_CY, 2, display::COL_BAD);
                morse::playWord(word);
                delay(1000);
                dlen = 0; decoded[0] = 0;
                drawWholeBase(word, decoded);
                lastAct = sys::now();
            }
        }
    }
}

static void playWholeWord() {
    const char* word = pickWord();
    while (true) {
        int r = playOneWholeWord(word);
        if (r == 0) return;
        word = pickWord();
    }
}

// --------------------------------------------------------------------------
//  Seletor de modo
// --------------------------------------------------------------------------
static const ui::Rect LEN_BTN[2] = { { 20, 52, 90, 38 }, { 130, 52, 90, 38 } };
static const ui::Rect CHK_BTN[2] = { { 20, 114, 90, 38 }, { 130, 114, 90, 38 } };
static const ui::Rect START_BTN  = { 40, 166, 160, 42 };

static void drawChooser() {
    Arduino_GFX* g = display::screen();
    g->fillScreen(display::COL_BG);
    display::text(i18n::t(i18n::MENU_WORDS), display::W / 2, 16, 2, display::COL_ACCENT);

    display::text(i18n::t(i18n::WORDLEN), display::W / 2, 42, 1, display::COL_DIM);
    ui::button(LEN_BTN[0], "3-4", display::COL_TEXT, !s_long);
    ui::button(LEN_BTN[1], "5-6", display::COL_TEXT,  s_long);

    display::text(i18n::t(i18n::CHECKMODE), display::W / 2, 104, 1, display::COL_DIM);
    ui::button(CHK_BTN[0], i18n::t(i18n::CHK_LETTER), display::COL_TEXT, !s_whole);
    ui::button(CHK_BTN[1], i18n::t(i18n::CHK_WORD),   display::COL_TEXT,  s_whole);

    ui::button(START_BTN, i18n::t(i18n::START), display::COL_GOOD, true);
    display::text(i18n::t(i18n::BACK_HINT), display::W / 2, 222, 1, display::COL_DIM);
}

// Retorna true p/ iniciar (com s_long/s_whole definidos), false se saiu.
static bool chooser() {
    drawChooser();
    while (true) {
        if (sys::wantExit()) return false;
        int x, y;
        if (sys::tapped(&x, &y)) {
            if (ui::hit(LEN_BTN[0], x, y)) { s_long = false; audio::sfxMenu(); drawChooser(); }
            if (ui::hit(LEN_BTN[1], x, y)) { s_long = true;  audio::sfxMenu(); drawChooser(); }
            if (ui::hit(CHK_BTN[0], x, y)) { s_whole = false; audio::sfxMenu(); drawChooser(); }
            if (ui::hit(CHK_BTN[1], x, y)) { s_whole = true;  audio::sfxMenu(); drawChooser(); }
            if (ui::hit(START_BTN, x, y))  { audio::sfxMenu(); return true; }
        }
        delay(8);
    }
}

void run() {
    if (!chooser()) return;
    if (s_whole) playWholeWord();
    else         playPerLetter();
}

} // namespace words
