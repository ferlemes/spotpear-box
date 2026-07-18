// ===========================================================================
//  pins.h — Mapa de GPIO da SpotPear / Waveshare ESP32-S3-Touch-LCD-1.54
//
//  Extraído do código-fonte oficial de exemplo do fabricante
//  (waveshareteam/ESP32-S3-Touch-LCD-1.54, exemplos 08_lvgl e 01_i2s_audio).
//  A SpotPear é uma variante do mesmo projeto e usa a mesma pinagem.
// ===========================================================================
#pragma once

// --- Display ST7789 (SPI de 4 fios) ---------------------------------------
#define LCD_DC    45
#define LCD_CS    21
#define LCD_SCK   38
#define LCD_MOSI  39
#define LCD_MISO  -1   // não usado (display é só escrita)
#define LCD_RST   40
#define LCD_BL    46   // backlight (luz de fundo)
#define LCD_W     240
#define LCD_H     240

// --- Barramento I2C (compartilhado: touch + IMU + codecs de áudio) --------
#define I2C_SDA   42
#define I2C_SCL   41

// --- Touch capacitivo CST816 (I2C) ----------------------------------------
#define TOUCH_INT 48
#define TOUCH_RST 47
#define TOUCH_ADDR 0x15

// --- IMU 6 eixos QMI8658 (I2C) — para uso futuro (chacoalhar p/ reiniciar) -
#define QMI8658_ADDR 0x6B

// --- Áudio: codec ES8311 (I2S) + amplificador NS4150B — fase 2 ------------
#define I2S_MCLK  8
#define I2S_BCLK  9
#define I2S_LRCK  10
#define I2S_DOUT  12
#define PA_CTRL   7    // habilita o amplificador do speaker

// --- Botões físicos do topo (diagnóstico + esquemático) -------------------
#define BTN_MINUS 0    // KEY_MINUS (também é o strap BOOT)
#define BTN_PLUS  4    // KEY_PLUS (livre para uso)
#define BTN_PWR   5    // KEY_PWR (SYS_OUT) — lê o botão de liga/desliga

// --- Gerência de energia (do esquemático) ---------------------------------
#define BAT_EN    2    // SYS_EN: HIGH trava a bateria ligada; LOW corta (desliga)
#define BAT_ADC   1    // tensão da bateria (ADC) — uso futuro
#define CHG_STAT  3    // status de carga — uso futuro

// --- Cartão microSD (modo SDMMC) — uso futuro -----------------------------
#define SD_CLK    16
#define SD_CMD    15
#define SD_D0     17
#define SD_D1     18
#define SD_D2     13
#define SD_D3     14
