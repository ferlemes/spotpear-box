// ===========================================================================
//  sys.h — Sistema compartilhado: energia (liga/desliga), botão "voltar" e
//  entrada de toque comum a todos os jogos.
// ===========================================================================
#pragma once
#include <stdint.h>

namespace sys {
    void begin();                     // trava a energia (BAT_EN) + pinos dos botões

    // Chame a cada iteração do loop de um jogo:
    //  - trata o botão PWR (segurar ~2s desliga de verdade);
    //  - retorna true se o botão "-" (KEY_MINUS) foi pressionado (= voltar).
    bool wantExit();

    // Tecla do Morse: true enquanto o botão "+" (KEY_PLUS) está pressionado.
    bool morseKeyDown();

    bool tapped(int* x, int* y);      // borda de um novo toque (dedo encostou)
    bool touching(int* x, int* y);    // dedo atualmente na tela (posição)

    uint32_t now();                   // millis()
}
