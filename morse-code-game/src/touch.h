// ===========================================================================
//  touch.h — Driver mínimo do controlador capacitivo CST816 (I2C).
//
//  O CST816 expõe a posição do dedo em registradores simples. Aqui lemos
//  por polling (sem depender do pino INT), o que basta para um jogo.
// ===========================================================================
#pragma once
#include <stdint.h>

namespace touch {
    // Inicializa I2C e dá reset no controlador. Chamar uma vez no setup().
    void begin();

    // Se há um dedo na tela, preenche x,y (0..239) e retorna true.
    bool read(uint16_t* x, uint16_t* y);
}
