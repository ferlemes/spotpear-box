// ===========================================================================
//  settings.h — Preferencias de jogo (nivel de velocidade da tecla).
//
//  O nivel define o limiar ponto/traco, os intervalos e a velocidade com que
//  o jogo TOCA o codigo. "Slow" = limiar grande (traco = segurar bastante),
//  ideal p/ iniciantes; "Fast" = limiar curto, p/ quem ja tem ritmo.
//  Em memoria apenas (volta ao padrao ao religar) — suficiente p/ o POC.
// ===========================================================================
#pragma once
#include <stdint.h>

namespace settings {
    // --- Velocidade da tecla (habilidade motora) --------------------------
    enum Speed { SLOW = 0, MEDIUM = 1, FAST = 2 };

    extern int speed;              // 0..2 (padrao SLOW)
    void setSpeed(int s);

    uint32_t dotMaxMs();           // segurar >= isso = traco
    uint32_t letterGapMs();        // tecla solta por tanto tempo = fim da letra
    int      playDotMs();          // "unidade" ao TOCAR o codigo (reproducao)

    const char* speedName(int s);  // "Slow" / "Med" / "Fast"

    // --- Nivel de dificuldade (conteudo: quais letras/palavras entram) ----
    //  Progressivo estilo Koch: o nivel define um TETO de letras (prefixo de
    //  morse::ORDER). Independente da velocidade.
    enum Level { BEGINNER = 0, INTER = 1, ADVANCED = 2 };

    extern int level;              // 0..2 (padrao BEGINNER)
    void setLevel(int s);

    int  letterCap();              // quantas letras de morse::ORDER o nivel cobre
    const char* levelName(int s);  // "Beg" / "Int" / "Adv"
}
