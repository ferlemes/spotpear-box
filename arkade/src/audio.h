// ===========================================================================
//  audio.h — Efeitos sonoros via codec ES8311 (I2S) + amplificador NS4150B.
// ===========================================================================
#pragma once
#include <stdint.h>

namespace audio {
    // Inicializa I2S + ES8311 + amplificador. Retorna false se o codec
    // não responder no I2C (o jogo segue funcionando, só mudo).
    bool begin();

    // Toca um tom senoidal (bloqueante). freq em Hz, dur em ms, vol 0..100.
    void tone(uint16_t freq, uint16_t ms, uint8_t vol = 80);

    // Efeitos prontos usados pelo jogo.
    void sfxMenu();   // ao escolher um modo no menu
    void sfxTap();    // ao marcar X/O
    void sfxWin();    // vitória (arpejo subindo)
    void sfxLose();   // derrota (descendo)
    void sfxDraw();   // empate (neutro)
    void sfxShake();    // ao chacoalhar para reiniciar
    void sfxShutdown(); // ao desligar (deep sleep)
}
