// ===========================================================================
//  morse.cpp — Tabela ITU A-Z, ordem de aprendizado e reproducao sonora.
// ===========================================================================
#include "morse.h"
#include "audio.h"
#include "settings.h"
#include <Arduino.h>
#include <string.h>

namespace morse {

// Padroes indexados por letra (A=0 .. Z=25).
static const char* TABLE[26] = {
    ".-",    // A
    "-...",  // B
    "-.-.",  // C
    "-..",   // D
    ".",     // E
    "..-.",  // F
    "--.",   // G
    "....",  // H
    "..",    // I
    ".---",  // J
    "-.-",   // K
    ".-..",  // L
    "--",    // M
    "-.",    // N
    "---",   // O
    ".--.",  // P
    "--.-",  // Q
    ".-.",   // R
    "...",   // S
    "-",     // T
    "..-",   // U
    "...-",  // V
    ".--",   // W
    "-..-",  // X
    "-.--",  // Y
    "--..",  // Z
};

// Vogais primeiro; depois consoantes numa ordem que vai das mais simples
// (poucos elementos) para as mais complexas.
const char* ORDER = "AEIOUTNMSRDKGWHBLFPVJCXYZQ";
int orderLen() { return 26; }

const char* code(char c) {
    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    if (c < 'A' || c > 'Z') return "";
    return TABLE[c - 'A'];
}

int rank(char c) {
    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    for (int i = 0; ORDER[i]; i++) if (ORDER[i] == c) return i;
    return 99;
}

char decode(const char* pat) {
    if (!pat || !pat[0]) return '?';
    for (int i = 0; i < 26; i++)
        if (strcmp(pat, TABLE[i]) == 0) return (char)('A' + i);
    return '?';
}

// --- Audio ----------------------------------------------------------------
//  A "unidade" (dot) vem do nivel de velocidade (settings::playDotMs()), para
//  a reproducao acompanhar o Slow/Med/Fast escolhido.
static const uint16_t FREQ = 700;    // sidetone classico

int dotMs() { return settings::playDotMs(); }

void playSymbol(char sym) {
    int u = settings::playDotMs();
    if (sym == '.')      audio::tone(FREQ, u, 80);
    else if (sym == '-') audio::tone(FREQ, u * 3, 80);
}

void playPattern(const char* pat) {
    if (!pat) return;
    int u = settings::playDotMs();
    for (int i = 0; pat[i]; i++) {
        playSymbol(pat[i]);
        if (pat[i + 1]) delay(u);           // gap de 1u entre elementos
    }
}

void playLetter(char c) { playPattern(code(c)); }

void playWord(const char* word) {
    if (!word) return;
    int u = settings::playDotMs();
    for (int i = 0; word[i]; i++) {
        char c = word[i];
        if (c == ' ') { delay(u * 7); continue; }        // gap de palavra
        playPattern(code(c));
        if (word[i + 1]) delay(u * 3);                   // gap de 3u entre letras
    }
}

} // namespace morse
