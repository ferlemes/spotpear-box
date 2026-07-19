// ===========================================================================
//  challenge.cpp — Implementacao do keyer + dica + avaliacao.
// ===========================================================================
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

namespace challenge {

// --- Ajustes de tempo (ms) ------------------------------------------------
// Limiar ponto/traco e gap de letra vem do nivel de velocidade (settings).
static const uint16_t SIDETONE_FREQ = 700;
static const uint32_t HINT_MS        = 3500;  // parado (sem digitar) = mostra dica
static const uint32_t WRONG_SHOW_MS  = 1500;  // tempo mostrando a correcao
static const uint32_t RELEASE_DB_MS  = 30;    // debounce da soltura (anti-jitter)

static const int BAND_H = 34;                 // faixa onde ficam os glifos

// Limpa a faixa dos glifos (entrada/dica).
static void clearBand(int cy) {
    display::screen()->fillRect(0, cy - BAND_H / 2, display::W, BAND_H, display::COL_BG);
}

// Desenha os pontos/tracos ja digitados.
static void drawEntered(const char* entered, int cx, int cy) {
    clearBand(cy);
    ui::drawMorse(entered, cx, cy, display::COL_DOT, display::COL_DASH);
}

// Mostra a dica: o codigo da letra em tom apagado + toca o som uma vez.
static void showHint(char target, int cx, int cy) {
    clearBand(cy);
    ui::drawMorse(morse::code(target), cx, cy, display::COL_DIM, display::COL_DIM);
    morse::playLetter(target);
}

int run(const Ctx& c) {
    c.redraw(c.user);              // pinta a tela-base (inclui KEY solta)

    char entered[8]; int elen = 0;
    entered[0] = 0;

    bool     keyDown   = false;
    uint32_t downStart = 0;
    bool     pendingUp = false;        // vendo a tecla solta, mas ainda no debounce
    uint32_t upAt      = 0;            // instante em que a tecla foi solta
    uint32_t lastAct   = sys::now();   // ultima atividade (p/ dica e gap de letra)
    bool     hintShown = false;
    char     lastProv  = 0;            // elemento "em curso" desenhado por ultimo

    while (true) {
        if (sys::wantExit()) return 0;      // botao de cima -> sair

        uint32_t now = sys::now();
        bool onKey = sys::morseKeyDown();      // tecla FISICA "+" (nunca o touch)

        if (onKey) {
            pendingUp = false;               // se era jitter, ignora
            if (!keyDown) {                 // borda de descida (apertou)
                keyDown = true;
                downStart = now;
                lastProv = 0;
                if (hintShown) { clearBand(c.glyphCy); hintShown = false; }
                audio::keyReset();
                ui::drawKey(true);
                Serial.println("[key] DOWN");
            }
            audio::feedKey(SIDETONE_FREQ, 80);   // sustenta o tom (~8ms)
            lastAct = now;

            // Feedback ao vivo: mostra o elemento em curso (ponto -> vira traco
            // quando o tempo segurado cruza o limiar). Redesenha so na virada.
            char prov = (now - downStart < settings::dotMaxMs()) ? '.' : '-';
            if (prov != lastProv) {
                clearBand(c.glyphCy);
                ui::drawMorseLive(entered, prov, c.glyphCx, c.glyphCy,
                                  display::COL_DOT, display::COL_DASH, display::COL_ACCENT);
                lastProv = prov;
            }
        } else if (keyDown) {
            // Tecla parece solta: so confirma apos o debounce (evita que uma
            // oscilacao do touch parta um traco em dois pontos).
            if (!pendingUp) { pendingUp = true; upAt = now; }
            if (now - upAt >= RELEASE_DB_MS) {
                keyDown = false; pendingUp = false;
                audio::keyOff();
                uint32_t held = upAt - downStart;    // tempo real de pressao
                char el = (held < settings::dotMaxMs()) ? '.' : '-';
                Serial.printf("[key] UP held=%lums -> %c\n", (unsigned long)held, el);
                if (elen < (int)sizeof(entered) - 1) {
                    entered[elen++] = el;
                    entered[elen] = 0;
                }
                ui::drawKey(false);
                drawEntered(entered, c.glyphCx, c.glyphCy);
                lastAct = now;
            } else {
                audio::feedKey(SIDETONE_FREQ, 80);   // mantem o tom no debounce
            }
        } else {
            delay(6);
        }

        // Fim da letra: teclou algo e ficou solto tempo suficiente -> avaliar.
        if (!keyDown && elen > 0 && (now - lastAct) > settings::letterGapMs()) {
            if (strcmp(entered, morse::code(c.target)) == 0) {
                audio::sfxGood();
                return 1;                    // acertou
            }
            // Errou: mostra a resposta certa e deixa tentar de novo.
            audio::sfxBad();
            clearBand(c.glyphCy);
            ui::drawMorse(morse::code(c.target), c.glyphCx, c.glyphCy,
                          display::COL_BAD, display::COL_BAD);
            // "Tente de novo" ACIMA da faixa (abaixo colidiria com o botao KEY).
            display::text(i18n::t(i18n::TRY_AGAIN), display::W / 2,
                          c.glyphCy - BAND_H / 2 - 6, 1, display::COL_BAD);
            delay(WRONG_SHOW_MS);
            c.redraw(c.user);
            elen = 0; entered[0] = 0;
            lastAct = sys::now(); hintShown = false;
        }

        // Dica: nada digitado e parado ha muito tempo.
        if (!keyDown && elen == 0 && !hintShown && (now - lastAct) > HINT_MS) {
            showHint(c.target, c.glyphCx, c.glyphCy);
            hintShown = true;
            lastAct = sys::now();
        }
    }
}

} // namespace challenge
