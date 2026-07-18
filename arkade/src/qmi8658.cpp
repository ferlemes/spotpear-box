// ===========================================================================
//  qmi8658.cpp — QMI8658 por I2C bruto (só acelerômetro).
//
//  Registradores usados:
//    0x00 WHO_AM_I (=0x05)   0x02 CTRL1   0x03 CTRL2 (accel)   0x08 CTRL7 (enable)
//    0x35.. AX_L,AX_H,AY_L,AY_H,AZ_L,AZ_H  (little-endian, int16)
//  Config: ±8g, 250 Hz -> sensibilidade 4096 LSB/g.
// ===========================================================================
#include "qmi8658.h"
#include "pins.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

namespace imu {

static const uint8_t REG_WHOAMI = 0x00;
static const uint8_t REG_CTRL1  = 0x02;
static const uint8_t REG_CTRL2  = 0x03;
static const uint8_t REG_CTRL3  = 0x04;
static const uint8_t REG_CTRL7  = 0x08;
static const uint8_t REG_RESET  = 0x60;
static const uint8_t REG_AX_L   = 0x35;

static const float    ACC_LSB_PER_G     = 4096.0f;   // ±8g
static const float    SHAKE_G           = 2.0f;      // limiar por amostra (calibrado)
static const uint32_t SHAKE_COOLDOWN_MS = 800;       // evita reinícios repetidos
static const uint32_t SHAKE_WINDOW_MS   = 500;       // janela p/ acumular amostras
static const uint32_t SHAKE_MIN_SPAN_MS = 120;       // movimento sustentado mínimo
static const int      SHAKE_HITS        = 3;         // nº de amostras > limiar exigidas

static uint8_t  addr      = QMI8658_ADDR;
static bool     ready     = false;
static uint32_t lastShake = 0;

static bool rd(uint8_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;      // repeated-start
    if (Wire.requestFrom((int)addr, (int)len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

static bool wr(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission(true) == 0;
}

bool begin() {
    // O endereço I2C depende do pino SA0; tenta 0x6B e 0x6A.
    const uint8_t cands[2] = { 0x6B, 0x6A };
    ready = false;
    for (uint8_t i = 0; i < 2; i++) {
        addr = cands[i];
        uint8_t id = 0;
        if (rd(REG_WHOAMI, &id, 1) && id == 0x05) { ready = true; break; }
    }
    if (!ready) return false;

    // Soft reset — ESSENCIAL. Sem ele o acelerômetro não converte e os
    // registradores de saída ficam congelados na primeira amostra.
    wr(REG_RESET, 0xB0);
    delay(20);

    wr(REG_CTRL1, 0x40);   // habilita oscilador interno (bit0=0) + auto-increment
    wr(REG_CTRL7, 0x00);   // desliga sensores durante a configuração
    wr(REG_CTRL2, 0x23);   // accel: ±8g (0x2<<4) | ODR 1000Hz (0x3)
    wr(REG_CTRL3, 0x00);   // giroscópio desligado
    wr(REG_CTRL7, 0x01);   // habilita só o acelerômetro
    delay(20);

    // Diagnóstico: confirma que a config pegou e que duas leituras já diferem.
    uint8_t c1 = 0, c2 = 0, c7 = 0;
    rd(REG_CTRL1, &c1, 1); rd(REG_CTRL2, &c2, 1); rd(REG_CTRL7, &c7, 1);
    float x1, y1, z1, x2, y2, z2;
    readAccelG(&x1, &y1, &z1);
    delay(30);
    readAccelG(&x2, &y2, &z2);
    Serial.printf("[imu] CTRL1=%02X CTRL2=%02X CTRL7=%02X  s1=(%.3f,%.3f,%.3f) s2=(%.3f,%.3f,%.3f)\n",
                  c1, c2, c7, x1, y1, z1, x2, y2, z2);
    return true;
}

bool readAccelG(float* x, float* y, float* z) {
    if (!ready) return false;
    uint8_t d[6];
    if (!rd(REG_AX_L, d, 6)) return false;
    int16_t rx = (int16_t)(d[0] | (d[1] << 8));
    int16_t ry = (int16_t)(d[2] | (d[3] << 8));
    int16_t rz = (int16_t)(d[4] | (d[5] << 8));
    if (x) *x = rx / ACC_LSB_PER_G;
    if (y) *y = ry / ACC_LSB_PER_G;
    if (z) *z = rz / ACC_LSB_PER_G;
    return true;
}

bool shaken(float* peakG) {
    if (!ready) return false;
    float x, y, z;
    if (!readAccelG(&x, &y, &z)) return false;
    float m = sqrtf(x * x + y * y + z * z);      // ~1.0g em repouso
    uint32_t now = millis();

    // Distingue chacoalhão de batida no dedo: exige VÁRIAS amostras acima do
    // limiar, espalhadas por >=SHAKE_MIN_SPAN_MS. Uma batida é um pico curto
    // (não acumula amostras ao longo do tempo); um chacoalhão é sustentado.
    static uint32_t winStart = 0;
    static int      hits     = 0;
    static float    peakM    = 0.0f;

    if (now - lastShake < SHAKE_COOLDOWN_MS) return false;   // ainda no cooldown

    if (m > SHAKE_G) {
        if (hits == 0 || now - winStart > SHAKE_WINDOW_MS) {
            winStart = now; hits = 1; peakM = m;             // inicia nova janela
        } else {
            hits++;
            if (m > peakM) peakM = m;
        }
        if (hits >= SHAKE_HITS && (now - winStart) >= SHAKE_MIN_SPAN_MS) {
            lastShake = now;
            hits = 0;
            if (peakG) *peakG = peakM;
            return true;
        }
    } else if (hits > 0 && now - winStart > SHAKE_WINDOW_MS) {
        hits = 0;                                            // janela expirou sem confirmar
    }
    return false;
}

} // namespace imu
