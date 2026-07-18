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
    //  - retorna true se o botão PLUS foi pressionado (= voltar para a home).
    bool wantExit();

    bool tapped(int* x, int* y);      // borda de um novo toque (dedo encostou)
    bool touching(int* x, int* y);    // dedo atualmente na tela (posição)

    uint32_t now();                   // millis()
}
