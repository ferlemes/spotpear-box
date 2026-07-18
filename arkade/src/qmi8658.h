// ===========================================================================
//  qmi8658.h — Driver mínimo do IMU 6 eixos QMI8658 (I2C).
//
//  Usamos só o acelerômetro, para detectar "chacoalhão" (shake) e reiniciar
//  a partida. Mesmo barramento I2C do touch (Wire já iniciado por touch::begin).
// ===========================================================================
#pragma once
#include <stdint.h>

namespace imu {
    bool begin();                                    // true se WHO_AM_I == 0x05
    bool readAccelG(float* x, float* y, float* z);   // aceleração em g
    bool shaken(float* peakG = nullptr);             // true 1x por chacoalhão
}
