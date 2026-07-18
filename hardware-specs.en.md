# Hardware Specification — SpotPear ESP32-S3-Touch-LCD-1.54

> Hardware reference for the **SpotPear ESP32-S3-Touch-LCD-1.54** device
> (pin-for-pin identical to the **Waveshare ESP32-S3-Touch-LCD-1.54**).
>
> The goal is to let any person — or any AI — build a brand-new project for this
> board from scratch using only this file, without needing the original source
> code. Every pin, I2C address, clock parameter, and critical init sequence is
> here, including the full **power-on / power-off / sleep lifecycle**.

---

## 1. Board identification

| Item | Value |
|---|---|
| Commercial name | SpotPear ESP32-S3-Touch-LCD-1.54 (variant of the Waveshare board of the same name) |
| SoC | ESP32-S3 (dual-core Xtensa LX7, Wi-Fi + Bluetooth LE) |
| Module / suffix | **N16R8** (a.k.a. **ESP32-S3R8**) |
| Flash | **16 MB**, **QIO** mode |
| PSRAM | **8 MB**, **octal / OPI** (hence the `R8` suffix) |
| USB | **Native USB** of the S3 (CDC). Enumerates as `/dev/ttyACM*` on the host |
| Display | **1.54"** IPS LCD, **240×240**, **ST7789** controller (4-wire SPI) |
| Touch | Capacitive **CST816** (I2C) |
| IMU | 6-axis accelerometer/gyroscope **QMI8658** (I2C) |
| Audio | **ES8311** codec (I2S) + **NS4150B** class-D amplifier + speaker |
| Storage | **microSD** slot (SDMMC 4-bit interface) |
| Battery | Li-ion/LiPo with on-board charger and a GPIO "power latch" |
| Buttons | 3 physical top buttons: `KEY_MINUS`, `KEY_PLUS`, `KEY_PWR` |

> **Mind the module variant:** only the **R8 (octal/OPI PSRAM)** version uses the
> memory configuration described here. If your board is R2 (quad/QSPI) or has no
> PSRAM, adjust `memory_type` and drop `-DBOARD_HAS_PSRAM`.

---

## 2. Full GPIO map (pinout)

All pins below were extracted from the manufacturer's official example code
(`waveshareteam/ESP32-S3-Touch-LCD-1.54`, examples `08_lvgl` and `01_i2s_audio`)
and validated on hardware.

| GPIO | Function | Peripheral | Notes |
|---:|---|---|---|
| 0  | `KEY_MINUS` | Button | **Also the BOOT strap** — be careful using it |
| 1  | `BAT_ADC` | Power | Battery voltage via ADC (÷3 divider) |
| 2  | `BAT_EN` (SYS_EN) | Power | **HIGH = latches the board ON; LOW = powers off** |
| 3  | `CHG_STAT` | Power | Charge status (open-drain, LOW = charging) |
| 4  | `KEY_PLUS` | Button | Free to use |
| 5  | `KEY_PWR` (SYS_OUT) | Button | Reads the power on/off button |
| 7  | `PA_CTRL` | Audio | Enables the NS4150B amplifier (HIGH = on) |
| 8  | `I2S_MCLK` | Audio | I2S master clock |
| 9  | `I2S_BCLK` | Audio | Bit clock (SCLK) |
| 10 | `I2S_LRCK` | Audio | Word select (LRCLK / WS) |
| 12 | `I2S_DOUT` | Audio | I2S data S3 → ES8311 |
| 13 | `SD_D2` | microSD | Data line 2 |
| 14 | `SD_D3` | microSD | Data line 3 |
| 15 | `SD_CMD` | microSD | Command |
| 16 | `SD_CLK` | microSD | Clock |
| 17 | `SD_D0` | microSD | Data line 0 |
| 18 | `SD_D1` | microSD | Data line 1 |
| 21 | `LCD_CS` | Display | Chip select |
| 38 | `LCD_SCK` | Display | SPI clock |
| 39 | `LCD_MOSI` | Display | SPI data (S3 → LCD) |
| 40 | `LCD_RST` | Display | Panel reset |
| 41 | `I2C_SCL` | I2C bus | Shared (touch + IMU + codec) |
| 42 | `I2C_SDA` | I2C bus | Shared (touch + IMU + codec) |
| 45 | `LCD_DC` | Display | Data/command |
| 46 | `LCD_BL` | Display | Backlight (HIGH = on) |
| 47 | `TOUCH_RST` | Touch | CST816 reset |
| 48 | `TOUCH_INT` | Touch | CST816 interrupt (optional; polling works too) |

> `LCD_MISO` is **not connected** (`-1`): the display is write-only.

### I2C bus — addresses

| Device | 7-bit address | Notes |
|---|---|---|
| Touch CST816 | `0x15` | — |
| IMU QMI8658 | `0x6B` (or `0x6A`) | Depends on the SA0 pin; probe both |
| Codec ES8311 | `0x18` (or `0x19`) | `0x18` with CE pin LOW, `0x19` with CE HIGH |

**Recommended speed:** 400 kHz (I2C fast mode). A single `Wire.begin(42, 41)`
serves all three devices.

---

## 3. Toolchain and build (PlatformIO)

Reference configuration (`platformio.ini`):

```ini
[env:spotpear-s3-154]
platform  = espressif32
board     = esp32-s3-devkitc-1
framework = arduino

; Memory: 16MB flash (QIO) + 8MB PSRAM octal (OPI)
board_build.arduino.memory_type = qio_opi
board_build.flash_mode          = qio
board_upload.flash_size         = 16MB
board_build.flash_size          = 16MB
board_build.partitions          = default_16MB.csv

; Native S3 USB-C → CDC on boot (shows up as /dev/ttyACM*)
build_flags =
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1

monitor_speed   = 115200
monitor_filters = esp32_exception_decoder, time

lib_deps =
    moononournation/GFX Library for Arduino @ 1.3.7
```

**Important compatibility notes:**

- **Core:** Arduino-ESP32 **2.0.17** (IDF 4.4). With this core, audio uses the **legacy I2S API** (`driver/i2s.h`). On core 3.x it would be the new API (`i2s_std.h`).
- **Arduino_GFX:** pinned at **1.3.7** because the 1.6.x line requires core 3.x (`esp32-hal-periman.h`). If you migrate to core 3.x you can bump the library.
- **USB:** if upload/monitor comes out on `/dev/ttyUSB*` (external UART chip instead of the native USB), remove the two `ARDUINO_USB_*` flags.

---

## 4. Display — ST7789 (240×240 IPS, 4-wire SPI)

| Parameter | Value |
|---|---|
| Controller | ST7789 |
| Resolution | 240 × 240 |
| Type | IPS (inverted colors → pass `IPS = true`) |
| Interface | 4-wire SPI (DC/CS/SCK/MOSI/RST), write-only |
| Backlight | GPIO 46, active HIGH |

### Initialization (Arduino_GFX 1.3.7)

```cpp
#include <Arduino_GFX_Library.h>

// DC=45, CS=21, SCK=38, MOSI=39, MISO=-1
Arduino_DataBus* bus = new Arduino_ESP32SPI(45, 21, 38, 39, -1);
// RST=40, rotation=0, IPS=true, width=240, height=240
Arduino_GFX* gfx = new Arduino_ST7789(bus, 40, 0, true, 240, 240);
gfx->begin();

pinMode(46, OUTPUT);        // backlight
digitalWrite(46, HIGH);     // turn on
gfx->fillScreen(gfx->color565(16, 18, 27));
```

Colors are **RGB565** (`gfx->color565(r, g, b)`). Rotation (`0..3`) sets the
logical screen orientation.

---

## 5. Capacitive touch — CST816 (I2C `0x15`)

Simple single-touch controller. It can be read by **I2C polling** (the INT line
is not required).

### Reset sequence (mandatory at boot)

```cpp
pinMode(TOUCH_RST, OUTPUT);   // GPIO 47
digitalWrite(TOUCH_RST, LOW);  delay(20);
digitalWrite(TOUCH_RST, HIGH); delay(60);
pinMode(TOUCH_INT, INPUT_PULLUP);   // GPIO 48 (optional)
Wire.begin(42, 41);                 // SDA=42, SCL=41
Wire.setClock(400000);
```

### Register map (read starting at `0x01`)

| Reg | Contents |
|---|---|
| `0x01` | GestureID |
| `0x02` | FingerNum (0 = no touch) |
| `0x03` | XposH (low 4 bits) |
| `0x04` | XposL |
| `0x05` | YposH (low 4 bits) |
| `0x06` | YposL |

Reading the position (X and Y are 12 bits):

```cpp
// Point to 0x01 and read 6 bytes at once (repeated-start)
Wire.beginTransmission(0x15);
Wire.write(0x01);
Wire.endTransmission(false);
Wire.requestFrom(0x15, 6);
uint8_t d[6];
for (int i = 0; i < 6; i++) d[i] = Wire.read();

uint8_t fingers = d[1];                                 // 0 = no finger
uint16_t x = ((uint16_t)(d[2] & 0x0F) << 8) | d[3];
uint16_t y = ((uint16_t)(d[4] & 0x0F) << 8) | d[5];
// x,y already match the 240×240 screen orientation (rotation 0)
```

---

## 6. 6-axis IMU — QMI8658 (I2C `0x6B`/`0x6A`)

Accelerometer + gyroscope. The address depends on the SA0 pin — probe `0x6B` and
`0x6A`. `WHO_AM_I` (reg `0x00`) must return **`0x05`**.

### Registers used

| Reg | Function |
|---|---|
| `0x00` | WHO_AM_I (= `0x05`) |
| `0x02` | CTRL1 (oscillator / auto-increment) |
| `0x03` | CTRL2 (accel config: range + ODR) |
| `0x04` | CTRL3 (gyro config) |
| `0x08` | CTRL7 (enable/disable sensors) |
| `0x35..0x3A` | AX_L, AX_H, AY_L, AY_H, AZ_L, AZ_H (int16 little-endian) |
| `0x60` | RESET (soft reset) |

### Init sequence (accel only, ±8g, ODR 1000 Hz)

```cpp
// 1) Soft reset — ESSENTIAL. Without it the accelerometer does not convert and
//    the output registers stay frozen on the first sample.
wr(0x60, 0xB0);  delay(20);

wr(0x02, 0x40);  // CTRL1: enable internal oscillator + auto-increment
wr(0x08, 0x00);  // CTRL7: disable sensors during configuration
wr(0x03, 0x23);  // CTRL2: accel ±8g (0x2<<4) | ODR 1000Hz (0x3)
wr(0x04, 0x00);  // CTRL3: gyroscope off
wr(0x08, 0x01);  // CTRL7: enable ONLY the accelerometer
delay(20);
```

### Converting to g

At **±8g** range, sensitivity is **4096 LSB/g**:

```cpp
int16_t raw = (int16_t)(lo | (hi << 8));
float g = raw / 4096.0f;    // acceleration in g
// magnitude ~1.0g at rest: m = sqrt(x*x + y*y + z*z)
```

Shake detection used in the project (to distinguish it from a finger tap):
require **≥3 samples** above **2.0 g** within a 500 ms window, sustained for at
least 120 ms, with an 800 ms cooldown between events.

---

## 7. Audio — ES8311 (I2S codec) + NS4150B (amplifier)

**Signal chain:** the ESP32-S3 is the **I2S master** and generates MCLK/BCLK/WS +
samples → the **ES8311** (slave, configured over I2C) converts to analog → the
**NS4150B** (enabled by `PA_CTRL`) drives the speaker.

### Audio pins

| Signal | GPIO |
|---|---|
| MCLK | 8 |
| BCLK (SCLK) | 9 |
| LRCK (WS) | 10 |
| DOUT (S3→codec) | 12 |
| PA_CTRL (amp enable) | 7 |

### Reference I2S parameters

| Parameter | Value |
|---|---|
| Sample rate | 16000 Hz |
| Bits per sample | 16 |
| MCLK | 256 × fs = **4.096 MHz** |
| Format | Standard I2S, stereo (L = R) |
| Mode | Master + TX |

### Initialization order (critical)

```cpp
pinMode(7, OUTPUT);
digitalWrite(7, LOW);    // 1) amplifier OFF during setup

initI2S();               // 2) install I2S driver (master, 16kHz/16bit, MCLK=256fs)
initCodec();             // 3) configure the ES8311 over I2C (uses the same Wire)

digitalWrite(7, HIGH);   // 4) only now turn the amplifier ON (avoids a "pop")
```

### Legacy I2S (core 2.0.17 / IDF 4.4)

```cpp
#include "driver/i2s.h"

i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = 16000,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,   // stereo (L=R)
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

### ES8311 codec configuration

The ES8311 is an I2S slave, with MCLK coming from the MCLK pin (GPIO 8). The
configuration used:

- MCLK = 4.096 MHz, fs = 16 kHz (a pair present in the driver's coefficient table)
- IN/OUT resolution = 16 bits
- DAC volume = 70/100; microphone off
- `es8311_create(I2C_NUM_0, 0x18)` → `es8311_init(...)` → `es8311_sample_frequency_config(...)` → `es8311_voice_volume_set(...)`

> **Two-stage volume:** on top of the codec DAC volume (0–100), the project also
> applies a **software gain** (`MASTER_GAIN ≈ 0.30`) on the waveform amplitude,
> because the speaker is tiny and saturates the amplifier at full scale. If it is
> too quiet, raise both; if it distorts, lower the software gain first.

The reference `es8311.c/.h` driver is Espressif's (Apache-2.0); its clock
coefficient table covers fs from 8k to 96k with several MCLKs.

---

## 8. Power lifecycle — power on / power off / sleep

This board uses a **software power latch**. There is no hard on/off switch: the
`KEY_PWR` button feeds the power supply while held, and the firmware must **latch
the rail on** by driving `BAT_EN` (GPIO 2) HIGH — otherwise, on battery, the board
dies the instant the button is released.

### Power pins

| Signal | GPIO | Function |
|---|---|---|
| `BAT_EN` (SYS_EN) | 2 | **HIGH = keep powered; LOW = cut the rail (power off)** |
| `KEY_PWR` (SYS_OUT) | 5 | Power button (LOW = pressed) |
| `BAT_ADC` | 1 | Battery voltage via ADC |
| `CHG_STAT` | 3 | Charge status (open-drain: **LOW = charging**) |

### 8.1 Power ON

1. The user presses **`KEY_PWR`**. The power path circuit energizes the board while the button is held.
2. Boot runs. **As the very first thing in `setup()`**, the firmware latches the rail:

```cpp
void setup() {
    // BEFORE ANYTHING ELSE: latch the battery rail ON. Otherwise, on battery,
    // the board only stays on while KEY_PWR is physically held down.
    pinMode(BAT_EN, OUTPUT);
    digitalWrite(BAT_EN, HIGH);          // <-- the "stay on" latch

    pinMode(BTN_PWR,  INPUT_PULLUP);     // GPIO 5
    pinMode(BTN_PLUS, INPUT_PULLUP);     // GPIO 4
    // ... rest of the init (display, touch, audio, imu, battery)
}
```

From here the user can release `KEY_PWR`; the board stays powered because
`BAT_EN` is HIGH.

### 8.2 Detecting the power-off request (long-press)

Each loop iteration polls `KEY_PWR`. A **~2-second hold** triggers shutdown. Note
the "armed" guard: right after power-on the button is still (or briefly) held, so
the code first waits for a **release** before it will arm the long-press timer —
this prevents an instant shutdown at boot.

```cpp
static void checkPower() {
    static bool     armed = false;      // becomes true after the first release
    static uint32_t pressStart = 0;
    static bool     wasDown = false;

    bool down = (digitalRead(BTN_PWR) == LOW);

    // Wait for the initial power-on press to be released before arming.
    if (!armed) { if (!down) armed = true; return; }

    uint32_t n = millis();
    if (down && !wasDown) pressStart = n;               // press edge
    if (down && (n - pressStart >= 2000)) powerOff();   // held ~2s → shut down
    wasDown = down;
}
```

### 8.3 Power OFF — real shutdown on battery, deep sleep on USB

Shutting down has two outcomes depending on how the board is powered:

- **On battery:** dropping `BAT_EN` LOW physically cuts the rail — the device turns fully off. Execution stops at that line.
- **On USB:** the 5 V rail stays alive regardless of `BAT_EN`, so the board cannot truly power off. As a fallback it enters **deep sleep**, configured to wake on `KEY_PWR`.

```cpp
#include "esp_sleep.h"
#include "driver/rtc_io.h"

static void powerOff() {
    display::drawSleepScreen();          // "see you!" screen
    audio::sfxShutdown();                // descending power-down jingle
    digitalWrite(PA_CTRL, LOW);          // mute/disable the amplifier first
    delay(150);

    digitalWrite(BAT_EN, LOW);           // ON BATTERY: the device powers off HERE

    // --- Only reached when powered over USB (5V still present) ---
    delay(1200);
    digitalWrite(LCD_BL, LOW);           // turn the backlight off

    // Wait for the button to be released so we don't wake immediately.
    while (digitalRead(BTN_PWR) == LOW) delay(10);

    // Configure KEY_PWR (GPIO 5) as the deep-sleep wake source.
    rtc_gpio_pullup_en((gpio_num_t)BTN_PWR);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_PWR, 0);  // wake when PWR = LOW (0)
    esp_deep_sleep_start();               // sleeps here; resets on wake
}
```

### 8.4 Waking from sleep

When in deep sleep (USB-powered case), pressing **`KEY_PWR`** pulls GPIO 5 LOW,
which is the `ext0` wake source. The chip wakes via a full reset and re-runs
`setup()` from the top — including the `BAT_EN = HIGH` latch — so the normal boot
flow resumes. There is no separate "resume" path; wake == fresh boot.

> **RTC wake pin note:** `KEY_PWR` is GPIO 5, which is a valid RTC/`ext0` wake pin
> on the ESP32-S3. `ext0` wakes on a specific level — here `0` (LOW) — so the
> internal RTC pull-up is enabled and the button-release wait avoids a bounce
> re-trigger.

### 8.5 Full power-lifecycle summary

```
        press KEY_PWR
             │
             ▼
   ┌───────────────────────┐
   │ boot → setup()        │
   │ BAT_EN = HIGH (latch) │◀──────────────┐  (deep-sleep wake = reset → setup)
   └───────────┬───────────┘               │
               │ running                    │
               ▼                            │
     hold KEY_PWR ~2s                       │
               │                            │
               ▼                            │
   ┌───────────────────────┐                │
   │ powerOff():           │                │
   │  PA_CTRL = LOW         │                │
   │  BAT_EN  = LOW ────────┼── on battery ──▶ FULLY OFF
   │  (USB only:)           │                │
   │  backlight off         │                │
   │  ext0 wake on KEY_PWR  │                │
   │  esp_deep_sleep_start()├── on USB ──────▶ DEEP SLEEP ──(press KEY_PWR)──┘
   └───────────────────────┘
```

---

## 9. Battery reading (÷3 resistive divider)

`BAT_ADC` measures **VBAT / 3** (divider R27 = 200 kΩ + R32 = 100 kΩ → factor 3.0).
Use the calibrated ADC (`analogReadMilliVolts`) for better accuracy:

```cpp
analogReadResolution(12);
analogSetPinAttenuation(BAT_ADC, ADC_11db);   // ~0..3.1 V range at the pin
pinMode(CHG_STAT, INPUT_PULLUP);

// Voltage (average of 16 samples):
uint32_t sum = 0;
for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(BAT_ADC);
float vbat = (sum / 16.0f) * 3.0f / 1000.0f;   // volts at the battery

// Percentage (simple linear curve):
//   V_FULL  = 4.20 V  (~100%)
//   V_EMPTY = 3.30 V  (~0%)
int pct = (vbat - 3.30f) / (4.20f - 3.30f) * 100.0f;   // clamp to 0..100

// Charging?
bool charging = (digitalRead(CHG_STAT) == LOW);   // open-drain
```

---

## 10. microSD (SDMMC 4-bit interface)

| Signal | GPIO |
|---|---|
| CLK | 16 |
| CMD | 15 |
| D0 | 17 |
| D1 | 18 |
| D2 | 13 |
| D3 | 14 |

Use **SDMMC** mode (4-bit bus). On Arduino-ESP32:
`SD_MMC.setPins(clk, cmd, d0, d1, d2, d3)` followed by `SD_MMC.begin()`. (This is
not the display's SPI bus.)

---

## 11. Physical buttons (top of the board)

| Button | GPIO | Use |
|---|---|---|
| `KEY_MINUS` | 0 | Free — **but also the BOOT strap**; keep it high at boot |
| `KEY_PLUS` | 4 | Free |
| `KEY_PWR` | 5 | Power on/off (reads SYS_OUT) |

All active LOW → use `INPUT_PULLUP`. A "click" is the **edge** of a press (compare
against the previous state). Holding `KEY_PWR` for ~2 s triggers shutdown
(see §8).

---

## 12. Straps, gotchas, and boot order

- **GPIO 0 = `KEY_MINUS` is the BOOT strap.** If it is LOW at reset, the board enters download mode instead of running firmware. Don't hold that button while powering up.
- **`BAT_EN` (GPIO 2) must go HIGH early.** On battery, without it, the board dies when the power button is released. Do it as the **first** action in `setup()` (see §8.1).
- **Recommended init order:** `power latch (BAT_EN)` → `Serial` → `display` → `touch (does Wire.begin)` → `audio (ES8311, uses the same Wire)` → `imu` → `battery`. `Wire.begin(42, 41)` must happen before configuring audio/IMU, since all three share the same I2C bus.
- **Octal PSRAM:** requires `memory_type = qio_opi` and `-DBOARD_HAS_PSRAM`. Verify with `ESP.getFreePsram()` (should report ~8 MB).
- **Amplifier (`PA_CTRL`):** keep it LOW until the codec is ready to avoid a speaker "pop." Cut it before powering off.
- **Native USB:** with `ARDUINO_USB_CDC_ON_BOOT=1`, `Serial` goes over the native USB-C (`/dev/ttyACM*`). Without those flags, it goes over an external UART bridge (`/dev/ttyUSB*`).

---

## 13. Pin `#define` summary (copy & paste)

```cpp
// --- Display ST7789 (4-wire SPI) ---
#define LCD_DC    45
#define LCD_CS    21
#define LCD_SCK   38
#define LCD_MOSI  39
#define LCD_MISO  -1     // unused (write-only)
#define LCD_RST   40
#define LCD_BL    46     // backlight (HIGH = on)
#define LCD_W     240
#define LCD_H     240

// --- I2C bus (touch + IMU + codec) ---
#define I2C_SDA   42
#define I2C_SCL   41

// --- Touch CST816 (I2C) ---
#define TOUCH_INT 48
#define TOUCH_RST 47
#define TOUCH_ADDR 0x15

// --- IMU QMI8658 (I2C) ---
#define QMI8658_ADDR 0x6B   // or 0x6A depending on SA0

// --- Audio: ES8311 (I2S) + NS4150B ---
#define I2S_MCLK  8
#define I2S_BCLK  9
#define I2S_LRCK  10
#define I2S_DOUT  12
#define PA_CTRL   7      // amplifier enable
#define ES8311_ADDR 0x18 // or 0x19 (CE pin)

// --- Physical buttons ---
#define BTN_MINUS 0      // KEY_MINUS (also BOOT strap)
#define BTN_PLUS  4      // KEY_PLUS
#define BTN_PWR   5      // KEY_PWR (SYS_OUT)

// --- Power ---
#define BAT_EN    2      // SYS_EN: HIGH latches on; LOW powers off
#define BAT_ADC   1      // battery voltage (VBAT/3)
#define CHG_STAT  3      // charge status (LOW = charging)

// --- microSD (SDMMC 4-bit) ---
#define SD_CLK    16
#define SD_CMD    15
#define SD_D0     17
#define SD_D1     18
#define SD_D2     13
#define SD_D3     14
```

---

## 14. Sources

- Pinout and drivers extracted from the manufacturer's official example code:
  `waveshareteam/ESP32-S3-Touch-LCD-1.54` (examples `08_lvgl` and `01_i2s_audio`).
- Reference ES8311 driver: Espressif Systems (Apache-2.0 license).
- Values validated in the "Arcade" firmware in this repository (`arkade/`), which
  runs on this board with Arduino-ESP32 2.0.17.
</content>
</invoke>
