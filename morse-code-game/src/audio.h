// ===========================================================================
//  audio.h — Som pelo codec ES8311 (I2S) + amplificador NS4150B.
//
//  Alem dos tons bloqueantes (menus, tocar o codigo morse), expoe um
//  "sidetone" NAO bloqueante: enquanto o jogador segura a tecla, o loop
//  chama feedKey() a cada iteracao para sustentar o tom em tempo real.
// ===========================================================================
#pragma once
#include <stdint.h>

namespace audio {
    bool begin();                                    // I2S + ES8311 + amp

    // Tom senoidal bloqueante. freq em Hz, dur em ms, vol 0..100.
    void tone(uint16_t freq, uint16_t ms, uint8_t vol = 80);

    // --- Sidetone (tom continuo enquanto a tecla esta pressionada) --------
    void keyReset();                                 // zera a fase (ao apertar)
    void feedKey(uint16_t freq, uint8_t vol = 80);   // alimenta ~8ms de tom
    void keyOff();                                   // rampa curta p/ evitar clique

    // Efeitos prontos.
    void sfxMenu();     // ao navegar no menu
    void sfxGood();     // acerto
    void sfxBad();      // erro
    void sfxLevelUp();  // subiu de nivel / completou palavra
    void sfxShutdown(); // ao desligar
}
