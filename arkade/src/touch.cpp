// ===========================================================================
//  touch.cpp — CST816 por I2C bruto.
//
//  Mapa de registradores relevante (leitura a partir de 0x01):
//    0x01 GestureID   0x02 FingerNum
//    0x03 XposH(4b)   0x04 XposL     -> X = ((XposH & 0x0F) << 8) | XposL
//    0x05 YposH(4b)   0x06 YposL     -> Y = ((YposH & 0x0F) << 8) | YposL
// ===========================================================================
#include "touch.h"
#include "pins.h"
#include <Arduino.h>
#include <Wire.h>

namespace touch {

void begin() {
    // Reset por hardware do controlador de toque.
    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);
    delay(20);
    digitalWrite(TOUCH_RST, HIGH);
    delay(60);

    pinMode(TOUCH_INT, INPUT_PULLUP);   // não usado no polling, mas deixa definido
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);              // I2C fast mode
}

bool read(uint16_t* x, uint16_t* y) {
    // Aponta para o registrador 0x01 e lê 6 bytes de uma vez.
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(0x01);
    if (Wire.endTransmission(false) != 0) return false;   // repeated-start

    if (Wire.requestFrom(TOUCH_ADDR, 6) != 6) return false;
    uint8_t d[6];
    for (int i = 0; i < 6; i++) d[i] = Wire.read();

    uint8_t fingers = d[1];
    if (fingers == 0) return false;     // nenhum dedo na tela

    uint16_t px = ((uint16_t)(d[2] & 0x0F) << 8) | d[3];
    uint16_t py = ((uint16_t)(d[4] & 0x0F) << 8) | d[5];

    // Segurança: descarta leituras fora da tela.
    if (px >= LCD_W || py >= LCD_H) return false;

    if (x) *x = px;
    if (y) *y = py;
    return true;
}

} // namespace touch
