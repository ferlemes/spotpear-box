# Especificação de Hardware — SpotPear ESP32-S3-Touch-LCD-1.54

> Documento de referência de hardware para o dispositivo **SpotPear ESP32-S3-Touch-LCD-1.54**
> (variante idêntica em pinagem à **Waveshare ESP32-S3-Touch-LCD-1.54**).
>
> O objetivo é permitir que qualquer pessoa — ou qualquer IA — construa um novo
> projeto do zero para esta placa apenas com este arquivo, sem precisar do
> código-fonte original. Todos os pinos, endereços, parâmetros de clock e
> sequências de inicialização críticas estão aqui.

---

## 1. Identificação da placa

| Item | Valor |
|---|---|
| Nome comercial | SpotPear ESP32-S3-Touch-LCD-1.54 (variante da Waveshare de mesmo nome) |
| SoC | ESP32-S3 (dual-core Xtensa LX7, Wi-Fi + Bluetooth LE) |
| Módulo / sufixo | **N16R8** (a.k.a. **ESP32-S3R8**) |
| Flash | **16 MB**, modo **QIO** |
| PSRAM | **8 MB**, **octal / OPI** (por isso o sufixo `R8`) |
| USB | USB-C **nativa do S3** (CDC). Enumera como `/dev/ttyACM*` no host |
| Tela | LCD IPS **1.54"**, **240×240**, controlador **ST7789** (SPI 4 fios) |
| Touch | Capacitivo **CST816** (I2C) |
| IMU | Acelerômetro/giroscópio 6 eixos **QMI8658** (I2C) |
| Áudio | Codec **ES8311** (I2S) + amplificador classe-D **NS4150B** + alto-falante |
| Armazenamento | Slot **microSD** (interface SDMMC 4 bits) |
| Bateria | Li-ion/LiPo com carregador embarcado e "power latch" via GPIO |
| Botões | 3 botões físicos no topo: `KEY_MINUS`, `KEY_PLUS`, `KEY_PWR` |

> **Atenção à variante do módulo:** só a versão **R8 (PSRAM octal/OPI)** usa a
> configuração de memória descrita aqui. Se a sua placa for R2 (quad/QSPI) ou
> sem PSRAM, ajuste `memory_type` e remova `-DBOARD_HAS_PSRAM`.

---

## 2. Mapa completo de GPIO (pinout)

Todos os pinos abaixo foram extraídos do código de exemplo oficial do fabricante
(`waveshareteam/ESP32-S3-Touch-LCD-1.54`, exemplos `08_lvgl` e `01_i2s_audio`) e
validados em hardware.

| GPIO | Função | Periférico | Observações |
|---:|---|---|---|
| 0  | `KEY_MINUS` | Botão | **Também é o strap BOOT** — cuidado ao usar |
| 1  | `BAT_ADC` | Energia | Tensão da bateria via ADC (divisor ÷3) |
| 2  | `BAT_EN` (SYS_EN) | Energia | **HIGH = trava a placa ligada; LOW = desliga** |
| 3  | `CHG_STAT` | Energia | Status de carga (dreno aberto, LOW = carregando) |
| 4  | `KEY_PLUS` | Botão | Livre para uso |
| 5  | `KEY_PWR` (SYS_OUT) | Botão | Lê o botão liga/desliga |
| 7  | `PA_CTRL` | Áudio | Habilita o amplificador NS4150B (HIGH = ligado) |
| 8  | `I2S_MCLK` | Áudio | Master clock do I2S |
| 9  | `I2S_BCLK` | Áudio | Bit clock (SCLK) |
| 10 | `I2S_LRCK` | Áudio | Word select (LRCLK / WS) |
| 12 | `I2S_DOUT` | Áudio | Dados I2S do S3 → ES8311 |
| 13 | `SD_D2` | microSD | Linha de dados 2 |
| 14 | `SD_D3` | microSD | Linha de dados 3 |
| 15 | `SD_CMD` | microSD | Comando |
| 16 | `SD_CLK` | microSD | Clock |
| 17 | `SD_D0` | microSD | Linha de dados 0 |
| 18 | `SD_D1` | microSD | Linha de dados 1 |
| 21 | `LCD_CS` | Display | Chip select |
| 38 | `LCD_SCK` | Display | SPI clock |
| 39 | `LCD_MOSI` | Display | SPI data (S3 → LCD) |
| 40 | `LCD_RST` | Display | Reset do painel |
| 41 | `I2C_SCL` | Barramento I2C | Compartilhado (touch + IMU + codec) |
| 42 | `I2C_SDA` | Barramento I2C | Compartilhado (touch + IMU + codec) |
| 45 | `LCD_DC` | Display | Data/command |
| 46 | `LCD_BL` | Display | Backlight (HIGH = aceso) |
| 47 | `TOUCH_RST` | Touch | Reset do CST816 |
| 48 | `TOUCH_INT` | Touch | Interrupção do CST816 (opcional; pode-se usar polling) |

> `LCD_MISO` **não é conectado** (`-1`): o display é somente escrita.

### Barramento I2C — endereços

| Dispositivo | Endereço 7-bit | Notas |
|---|---|---|
| Touch CST816 | `0x15` | — |
| IMU QMI8658 | `0x6B` (ou `0x6A`) | Depende do pino SA0; tente ambos |
| Codec ES8311 | `0x18` (ou `0x19`) | `0x18` com pino CE em LOW, `0x19` em HIGH |

**Velocidade recomendada:** 400 kHz (I2C fast mode). Um único `Wire.begin(42, 41)`
serve para todos os três dispositivos.

---

## 3. Toolchain e build (PlatformIO)

Configuração de referência (`platformio.ini`):

```ini
[env:spotpear-s3-154]
platform  = espressif32
board     = esp32-s3-devkitc-1
framework = arduino

; Memória: 16MB flash (QIO) + 8MB PSRAM octal (OPI)
board_build.arduino.memory_type = qio_opi
board_build.flash_mode          = qio
board_upload.flash_size         = 16MB
board_build.flash_size          = 16MB
board_build.partitions          = default_16MB.csv

; USB-C nativa do S3 → CDC on boot (aparece como /dev/ttyACM*)
build_flags =
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1

monitor_speed   = 115200
monitor_filters = esp32_exception_decoder, time

lib_deps =
    moononournation/GFX Library for Arduino @ 1.3.7
```

**Notas de compatibilidade importantes:**

- **Core:** Arduino-ESP32 **2.0.17** (IDF 4.4). Com esse core, o áudio usa a **API I2S legada** (`driver/i2s.h`). No core 3.x seria a API nova (`i2s_std.h`).
- **Arduino_GFX:** fixado em **1.3.7** porque a série 1.6.x exige core 3.x (`esp32-hal-periman.h`). Se migrar para o core 3.x, pode subir a versão da lib.
- **USB:** se o upload/monitor sair por `/dev/ttyUSB*` (chip UART externo em vez da USB nativa), remova as duas flags `ARDUINO_USB_*`.

---

## 4. Display — ST7789 (240×240 IPS, SPI 4 fios)

| Parâmetro | Valor |
|---|---|
| Controlador | ST7789 |
| Resolução | 240 × 240 |
| Tipo | IPS (cores invertidas → passar `IPS = true`) |
| Interface | SPI 4 fios (DC/CS/SCK/MOSI/RST), somente escrita |
| Backlight | GPIO 46, ativo em HIGH |

### Inicialização (Arduino_GFX 1.3.7)

```cpp
#include <Arduino_GFX_Library.h>

// DC=45, CS=21, SCK=38, MOSI=39, MISO=-1
Arduino_DataBus* bus = new Arduino_ESP32SPI(45, 21, 38, 39, -1);
// RST=40, rotação=0, IPS=true, largura=240, altura=240
Arduino_GFX* gfx = new Arduino_ST7789(bus, 40, 0, true, 240, 240);
gfx->begin();

pinMode(46, OUTPUT);        // backlight
digitalWrite(46, HIGH);     // acende
gfx->fillScreen(gfx->color565(16, 18, 27));
```

Cores em **RGB565** (`gfx->color565(r, g, b)`). A rotação (`0..3`) gira a
orientação lógica da tela.

---

## 5. Touch capacitivo — CST816 (I2C `0x15`)

Controlador de toque simples, com 1 ponto de contato. Pode ser lido por **polling
por I2C** (não precisa da linha INT).

### Sequência de reset (obrigatória no boot)

```cpp
pinMode(TOUCH_RST, OUTPUT);   // GPIO 47
digitalWrite(TOUCH_RST, LOW);  delay(20);
digitalWrite(TOUCH_RST, HIGH); delay(60);
pinMode(TOUCH_INT, INPUT_PULLUP);   // GPIO 48 (opcional)
Wire.begin(42, 41);                 // SDA=42, SCL=41
Wire.setClock(400000);
```

### Mapa de registradores (leitura a partir de `0x01`)

| Reg | Conteúdo |
|---|---|
| `0x01` | GestureID |
| `0x02` | FingerNum (0 = sem toque) |
| `0x03` | XposH (4 bits baixos) |
| `0x04` | XposL |
| `0x05` | YposH (4 bits baixos) |
| `0x06` | YposL |

Leitura de posição (X e Y são 12 bits):

```cpp
// Aponta para 0x01 e lê 6 bytes de uma vez (repeated-start)
Wire.beginTransmission(0x15);
Wire.write(0x01);
Wire.endTransmission(false);
Wire.requestFrom(0x15, 6);
uint8_t d[6];
for (int i = 0; i < 6; i++) d[i] = Wire.read();

uint8_t fingers = d[1];                                 // 0 = sem dedo
uint16_t x = ((uint16_t)(d[2] & 0x0F) << 8) | d[3];
uint16_t y = ((uint16_t)(d[4] & 0x0F) << 8) | d[5];
// x,y já vêm na mesma orientação da tela 240×240 (rotação 0)
```

---

## 6. IMU 6 eixos — QMI8658 (I2C `0x6B`/`0x6A`)

Acelerômetro + giroscópio. O endereço depende do pino SA0 — sonde `0x6B` e `0x6A`.
`WHO_AM_I` (reg `0x00`) deve retornar **`0x05`**.

### Registradores usados

| Reg | Função |
|---|---|
| `0x00` | WHO_AM_I (= `0x05`) |
| `0x02` | CTRL1 (oscilador / auto-increment) |
| `0x03` | CTRL2 (config do acelerômetro: faixa + ODR) |
| `0x04` | CTRL3 (config do giroscópio) |
| `0x08` | CTRL7 (habilita/desabilita sensores) |
| `0x35..0x3A` | AX_L, AX_H, AY_L, AY_H, AZ_L, AZ_H (int16 little-endian) |
| `0x60` | RESET (soft reset) |

### Sequência de inicialização (só acelerômetro, ±8g, ODR 1000 Hz)

```cpp
// 1) Soft reset — ESSENCIAL. Sem ele o acelerômetro não converte e os
//    registradores de saída ficam congelados na primeira amostra.
wr(0x60, 0xB0);  delay(20);

wr(0x02, 0x40);  // CTRL1: habilita oscilador interno + auto-increment
wr(0x08, 0x00);  // CTRL7: desliga sensores durante a configuração
wr(0x03, 0x23);  // CTRL2: accel ±8g (0x2<<4) | ODR 1000Hz (0x3)
wr(0x04, 0x00);  // CTRL3: giroscópio desligado
wr(0x08, 0x01);  // CTRL7: habilita SÓ o acelerômetro
delay(20);
```

### Conversão para g

Com faixa **±8g**, a sensibilidade é **4096 LSB/g**:

```cpp
int16_t raw = (int16_t)(lo | (hi << 8));
float g = raw / 4096.0f;    // aceleração em g
// magnitude ~1.0g em repouso: m = sqrt(x*x + y*y + z*z)
```

Detecção de "chacoalhão" usada no projeto (para distinguir de uma batida): exigir
**≥3 amostras** acima de **2.0 g** dentro de uma janela de 500 ms, com duração
mínima sustentada de 120 ms, e um cooldown de 800 ms entre eventos.

---

## 7. Áudio — ES8311 (codec I2S) + NS4150B (amplificador)

**Cadeia de sinal:** o ESP32-S3 é **mestre I2S** e gera MCLK/BCLK/WS + amostras →
o **ES8311** (escravo, configurado por I2C) converte para analógico → o **NS4150B**
(habilitado por `PA_CTRL`) amplifica para o alto-falante.

### Pinos de áudio

| Sinal | GPIO |
|---|---|
| MCLK | 8 |
| BCLK (SCLK) | 9 |
| LRCK (WS) | 10 |
| DOUT (S3→codec) | 12 |
| PA_CTRL (liga o amp) | 7 |

### Parâmetros I2S de referência

| Parâmetro | Valor |
|---|---|
| Sample rate | 16000 Hz |
| Bits por amostra | 16 |
| MCLK | 256 × fs = **4.096 MHz** |
| Formato | I2S padrão, estéreo (L = R) |
| Modo | Master + TX |

### Ordem de inicialização (crítica)

```cpp
pinMode(7, OUTPUT);
digitalWrite(7, LOW);    // 1) amplificador DESLIGADO durante o setup

initI2S();               // 2) instala driver I2S (mestre, 16kHz/16bit, MCLK=256fs)
initCodec();             // 3) configura o ES8311 por I2C (usa o mesmo Wire)

digitalWrite(7, HIGH);   // 4) só então LIGA o amplificador (evita "pop")
```

### I2S legado (core 2.0.17 / IDF 4.4)

```cpp
#include "driver/i2s.h"

i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = 16000,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,   // estéreo (L=R)
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 256,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
    .fixed_mclk           = 0,
    .mclk_multiple        = I2S_MCLK_MULTIPLE_256,
    .bits_per_chan        = I2S_BITS_PER_CHAN_16BIT,
};
i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);

i2s_pin_config_t pins = {
    .mck_io_num   = 8,
    .bck_io_num   = 9,
    .ws_io_num    = 10,
    .data_out_num = 12,
    .data_in_num  = I2S_PIN_NO_CHANGE,
};
i2s_set_pin(I2S_NUM_0, &pins);
i2s_zero_dma_buffer(I2S_NUM_0);
```

### Configuração do codec ES8311

O ES8311 é escravo I2S, MCLK vindo do pino MCLK (GPIO 8). Config usada:

- MCLK = 4.096 MHz, fs = 16 kHz (par presente na tabela de coeficientes do driver)
- Resolução IN/OUT = 16 bits
- Volume do DAC = 70/100; microfone desligado
- `es8311_create(I2C_NUM_0, 0x18)` → `es8311_init(...)` → `es8311_sample_frequency_config(...)` → `es8311_voice_volume_set(...)`

> **Volume em dois estágios:** além do volume do DAC do codec (0–100), o projeto
> aplica um **ganho de software** (`MASTER_GAIN ≈ 0.30`) na amplitude da onda,
> porque o alto-falante é pequeno e satura o amplificador em amplitude cheia. Se o
> som ficar baixo, suba os dois; se distorcer, baixe o ganho de software primeiro.

O driver `es8311.c/.h` de referência é o da Espressif (Apache-2.0); a tabela de
coeficientes de clock cobre fs de 8k a 96k com vários MCLKs.

---

## 8. Gerência de energia e bateria

Esta placa tem **power latch por software**: ao ligar (segurando o `KEY_PWR`), o
firmware precisa **travar a alimentação** colocando `BAT_EN` (GPIO 2) em HIGH,
senão a placa desliga assim que o botão é solto (quando alimentada por bateria).

### Pinos de energia

| Sinal | GPIO | Função |
|---|---|---|
| `BAT_EN` (SYS_EN) | 2 | **HIGH = mantém ligado; LOW = corta a energia (desliga)** |
| `BAT_ADC` | 1 | Tensão da bateria via ADC |
| `CHG_STAT` | 3 | Status de carga (dreno aberto: **LOW = carregando**) |
| `KEY_PWR` | 5 | Botão liga/desliga (LOW = pressionado) |

Não existe chave física de liga/desliga: o botão `KEY_PWR` alimenta a fonte
enquanto pressionado, e o firmware precisa **travar o rail** com `BAT_EN` HIGH.

### 8.1 Ligar

1. O usuário pressiona **`KEY_PWR`**. O circuito de energia energiza a placa enquanto o botão fica pressionado.
2. O boot roda. **Como PRIMEIRA coisa do `setup()`**, o firmware trava o rail:

```cpp
void setup() {
    // ANTES DE TUDO: trava o rail da bateria ligado. Senão, na bateria, a placa
    // só fica ligada enquanto o KEY_PWR estiver fisicamente pressionado.
    pinMode(BAT_EN, OUTPUT);
    digitalWrite(BAT_EN, HIGH);          // <-- a trava de "continuar ligado"

    pinMode(BTN_PWR,  INPUT_PULLUP);     // GPIO 5
    pinMode(BTN_PLUS, INPUT_PULLUP);     // GPIO 4
    // ... resto da init (display, touch, audio, imu, battery)
}
```

A partir daqui o usuário pode soltar o `KEY_PWR`; a placa segue ligada porque
`BAT_EN` está em HIGH.

### 8.2 Detectar o pedido de desligar (segurar ~2s)

A cada iteração do loop, sonda-se o `KEY_PWR`. Segurar por **~2 segundos** dispara
o desligamento. Repare na trava `armed`: logo após ligar o botão ainda está (ou
esteve) pressionado, então o código primeiro espera a **soltura** antes de armar o
cronômetro do long-press — isso evita um desligamento instantâneo no boot.

```cpp
static void checkPower() {
    static bool     armed = false;      // vira true após a primeira soltura
    static uint32_t pressStart = 0;
    static bool     wasDown = false;

    bool down = (digitalRead(BTN_PWR) == LOW);

    // Espera a soltura do aperto inicial (o de ligar) antes de armar.
    if (!armed) { if (!down) armed = true; return; }

    uint32_t n = millis();
    if (down && !wasDown) pressStart = n;               // borda de aperto
    if (down && (n - pressStart >= 2000)) powerOff();   // segurou ~2s → desliga
    wasDown = down;
}
```

### 8.3 Desligar — desliga de verdade na bateria, deep sleep no USB

O desligamento tem dois desfechos, dependendo de como a placa está alimentada:

- **Na bateria:** colocar `BAT_EN` em LOW corta fisicamente o rail — o aparelho desliga por completo. A execução para nessa linha.
- **No USB:** os 5 V continuam presentes independentemente do `BAT_EN`, então não dá para desligar de verdade. Como alternativa, entra em **deep sleep**, configurado para acordar no `KEY_PWR`.

```cpp
#include "esp_sleep.h"
#include "driver/rtc_io.h"

static void powerOff() {
    display::drawSleepScreen();          // tela "até mais!"
    audio::sfxShutdown();                // jingle descendente de power-down
    digitalWrite(PA_CTRL, LOW);          // corta o amplificador primeiro
    delay(150);

    digitalWrite(BAT_EN, LOW);           // NA BATERIA: o aparelho desliga AQUI

    // --- Só chega abaixo se estiver alimentado por USB (os 5V seguem) ---
    delay(1200);
    digitalWrite(LCD_BL, LOW);           // apaga o backlight

    // Espera o botão ser solto para não acordar na hora.
    while (digitalRead(BTN_PWR) == LOW) delay(10);

    // Configura o KEY_PWR (GPIO 5) como fonte de wake do deep sleep.
    rtc_gpio_pullup_en((gpio_num_t)BTN_PWR);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_PWR, 0);  // acorda com PWR = LOW (0)
    esp_deep_sleep_start();               // dorme aqui; ao acordar dá reset
}
```

### 8.4 Acordar do sleep

No deep sleep (caso alimentado por USB), pressionar **`KEY_PWR`** puxa o GPIO 5 para
LOW, que é a fonte de wake `ext0`. O chip acorda via um reset completo e roda o
`setup()` do início — incluindo a trava `BAT_EN = HIGH` — então o fluxo normal de
boot recomeça. Não há um caminho separado de "resume"; acordar == boot novo.

> **Nota sobre o pino de wake:** `KEY_PWR` é o GPIO 5, um pino RTC/`ext0` válido no
> ESP32-S3. O `ext0` acorda em um nível específico — aqui `0` (LOW) — por isso o
> pull-up interno do RTC é habilitado e a espera pela soltura do botão evita um
> re-disparo por bounce.

### 8.5 Resumo do ciclo de energia

```
        aperta KEY_PWR
             │
             ▼
   ┌───────────────────────┐
   │ boot → setup()        │
   │ BAT_EN = HIGH (trava) │◀──────────────┐  (acordar do deep sleep = reset → setup)
   └───────────┬───────────┘               │
               │ rodando                    │
               ▼                            │
     segura KEY_PWR ~2s                     │
               │                            │
               ▼                            │
   ┌───────────────────────┐                │
   │ powerOff():           │                │
   │  PA_CTRL = LOW         │                │
   │  BAT_EN  = LOW ────────┼── na bateria ──▶ DESLIGADO DE VERDADE
   │  (só no USB:)          │                │
   │  apaga backlight       │                │
   │  wake ext0 no KEY_PWR  │                │
   │  esp_deep_sleep_start()├── no USB ──────▶ DEEP SLEEP ──(aperta KEY_PWR)──┘
   └───────────────────────┘
```

### Leitura da bateria (divisor resistivo ÷3)

O `BAT_ADC` mede **VBAT / 3** (divisor R27 = 200 kΩ + R32 = 100 kΩ → fator 3.0).
Use o ADC calibrado (`analogReadMilliVolts`) para melhor precisão:

```cpp
analogReadResolution(12);
analogSetPinAttenuation(BAT_ADC, ADC_11db);   // faixa ~0..3.1 V no pino
pinMode(CHG_STAT, INPUT_PULLUP);

// Tensão (média de 16 amostras):
uint32_t sum = 0;
for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(BAT_ADC);
float vbat = (sum / 16.0f) * 3.0f / 1000.0f;   // volts na bateria

// Percentual (curva linear simples):
//   V_FULL  = 4.20 V  (~100%)
//   V_EMPTY = 3.30 V  (~0%)
int pct = (vbat - 3.30f) / (4.20f - 3.30f) * 100.0f;   // limitar a 0..100

// Carregando?
bool charging = (digitalRead(CHG_STAT) == LOW);   // dreno aberto
```

---

## 9. microSD (interface SDMMC 4 bits)

| Sinal | GPIO |
|---|---|
| CLK | 16 |
| CMD | 15 |
| D0 | 17 |
| D1 | 18 |
| D2 | 13 |
| D3 | 14 |

Usar em modo **SDMMC** (barramento de 4 bits). No Arduino-ESP32, `SD_MMC.setPins(clk, cmd, d0, d1, d2, d3)` seguido de `SD_MMC.begin()`. (Não é o mesmo barramento SPI do display.)

---

## 10. Botões físicos (topo da placa)

| Botão | GPIO | Uso |
|---|---|---|
| `KEY_MINUS` | 0 | Livre — **mas é também o strap de BOOT**; mantenha em nível alto no boot |
| `KEY_PLUS` | 4 | Livre |
| `KEY_PWR` | 5 | Liga/desliga (lê o SYS_OUT) |

Todos ativos em LOW → use `INPUT_PULLUP`. Um "clique" é a **borda** de pressionar
(comparar com o estado anterior). Segurar `KEY_PWR` por ~2 s aciona o desligamento.

---

## 11. Straps, armadilhas e ordem de boot

- **GPIO 0 = `KEY_MINUS` é o strap de BOOT.** Se estiver em LOW no reset, a placa entra em modo de download em vez de rodar o firmware. Evite segurar esse botão ao energizar.
- **`BAT_EN` (GPIO 2) precisa ir a HIGH cedo.** Na bateria, sem isso, a placa desliga ao soltar o botão de power. Faça isso como **primeira** ação do `setup()`.
- **Ordem recomendada de init:** `power latch (BAT_EN)` → `Serial` → `display` → `touch (faz Wire.begin)` → `audio (ES8311, usa o mesmo Wire)` → `imu` → `battery`. O `Wire.begin(42, 41)` deve ocorrer antes de configurar áudio/IMU, pois todos dividem o mesmo barramento I2C.
- **PSRAM octal:** exige `memory_type = qio_opi` e `-DBOARD_HAS_PSRAM`. Confirme com `ESP.getFreePsram()` (deve reportar ~8 MB).
- **Amplificador (`PA_CTRL`):** mantenha em LOW até o codec estar pronto, para evitar "pop" no alto-falante. Corte-o antes de desligar.
- **USB nativa:** com `ARDUINO_USB_CDC_ON_BOOT=1`, a `Serial` sai pela USB-C nativa (`/dev/ttyACM*`). Sem essas flags, sai por um conversor UART externo (`/dev/ttyUSB*`).

---

## 12. Resumo dos `#define` de pinos (copiar e colar)

```cpp
// --- Display ST7789 (SPI 4 fios) ---
#define LCD_DC    45
#define LCD_CS    21
#define LCD_SCK   38
#define LCD_MOSI  39
#define LCD_MISO  -1     // não usado (só escrita)
#define LCD_RST   40
#define LCD_BL    46     // backlight (HIGH = aceso)
#define LCD_W     240
#define LCD_H     240

// --- Barramento I2C (touch + IMU + codec) ---
#define I2C_SDA   42
#define I2C_SCL   41

// --- Touch CST816 (I2C) ---
#define TOUCH_INT 48
#define TOUCH_RST 47
#define TOUCH_ADDR 0x15

// --- IMU QMI8658 (I2C) ---
#define QMI8658_ADDR 0x6B   // ou 0x6A conforme SA0

// --- Áudio: ES8311 (I2S) + NS4150B ---
#define I2S_MCLK  8
#define I2S_BCLK  9
#define I2S_LRCK  10
#define I2S_DOUT  12
#define PA_CTRL   7      // habilita o amplificador
#define ES8311_ADDR 0x18 // ou 0x19 (pino CE)

// --- Botões físicos ---
#define BTN_MINUS 0      // KEY_MINUS (também strap BOOT)
#define BTN_PLUS  4      // KEY_PLUS
#define BTN_PWR   5      // KEY_PWR (SYS_OUT)

// --- Energia ---
#define BAT_EN    2      // SYS_EN: HIGH trava ligado; LOW desliga
#define BAT_ADC   1      // tensão da bateria (VBAT/3)
#define CHG_STAT  3      // status de carga (LOW = carregando)

// --- microSD (SDMMC 4 bits) ---
#define SD_CLK    16
#define SD_CMD    15
#define SD_D0     17
#define SD_D1     18
#define SD_D2     13
#define SD_D3     14
```

---

## 13. Fontes

- Pinagem e drivers extraídos do código-fonte de exemplo oficial do fabricante:
  `waveshareteam/ESP32-S3-Touch-LCD-1.54` (exemplos `08_lvgl` e `01_i2s_audio`).
- Driver ES8311 de referência: Espressif Systems (licença Apache-2.0).
- Valores validados no firmware "Arcade" deste repositório (`arkade/`), que roda
  nesta placa com Arduino-ESP32 2.0.17.
</content>
</invoke>
