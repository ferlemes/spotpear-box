# Morse Code Game — SpotPear ESP32-S3-Touch-LCD-1.54

A little game to **learn Morse code**, for the same board as the `arkade`
project (ESP32-S3, 240×240 IPS touch LCD, ES8311 speaker).

The UI is in **English by default** and can switch to **Portuguese** or
**Spanish** in *Settings* (letters and Morse are universal; only the labels and
the word list change).

## How it works

Morse is keyed on a **physical button — never the touchscreen**. The board has
three top buttons; this game maps them as:

| Button | GPIO | Role |
|--------|------|------|
| **`KEY_PLUS` ("+")**  | 4 | **The Morse key** — short press = dot (·), hold = dash (−) |
| **`KEY_MINUS` ("−")** | 0 | **Back** to the menu (from any mode) |
| `KEY_PWR`             | 5 | Power (hold ~2 s to turn off) |

- You hear a live sidetone (~700 Hz) while the "+" key is held.
- The on-screen **KEY [+]** box is only an *indicator*: it lights up while the
  physical "+" button is pressed. It is not tap-able.
- Wait a moment after the last element and the letter is evaluated.
- **Take too long?** A hint appears (the letter's code) and it's played once.

The touchscreen is still used for **menu navigation** (choosing a mode, picking
a language, Hear/Next in Learn, the word options in Listen) — just not for
keying Morse.

## Modes

1. **Learn** — how to play, then a tour of every letter with its code and sound.
   Starts with the vowels (A E I O U), then introduces the rest gradually.
2. **Letters** — a letter is shown, you key it. Starts with vowels only and
   unlocks more letters as you get answers right.
3. **Words** — encode words in the chosen language. On entry, a chooser picks
   two independent options:
   - **Length**: `3-4` (short, filtered by the difficulty level) or `5-6`
     (long words — the "level 2" of the word game, using the full alphabet).
   - **Check**: `Letter` (checks and hints each letter, like before) or `Word`
     (spell the **whole word** first — no per-letter help — and it verifies only
     at the end; on a miss it shows and plays the correct word).
4. **Listen** (reverse) — the game plays the sound and shows the dots/dashes;
   you pick the right word from four options. *(Replay button repeats it.)*

## Difficulty & speed (Settings — two independent axes)

- **Level** (content): `Beg` / `Int` / `Adv`. A level caps which letters are in
  play — a prefix of the learning order (Beg = 10 letters incl. vowels, Int =
  18, Adv = 26), Koch-style. In *Letters* the pool still grows gradually **up to
  that cap**; *Words* and *Listen* only use words whose letters are all within
  the level; *Learn* walks only the level's letters.
- **Speed** (motor skill): `Slow` / `Med` / `Fast`. Sets the dot/dash threshold,
  the letter gap and the playback speed. Default **Slow** (dash = hold ≥ 400 ms)
  for a wide, reliable dot/dash separation. While you hold the key, the current
  element is shown live and flips to a dash the moment it crosses the threshold.

Both reset to defaults on power-cycle (in-memory only for now).

## Build / flash

Same toolchain as `arkade` (PlatformIO, Arduino-ESP32 2.0.17):

```bash
~/.pio-venv/bin/pio run                 # build
~/.pio-venv/bin/pio run -t upload       # flash (USB-C = native /dev/ttyACM*)
~/.pio-venv/bin/pio device monitor      # serial @ 115200
```

## Tuning

- **Speed levels** — `src/settings.cpp` tables `DOTMAX` / `LGAP` / `PLAYDOT`
  (dot/dash threshold, letter gap, playback unit per Slow/Med/Fast).
- **Difficulty levels** — `src/settings.cpp` `CAP` (letters per Beg/Int/Adv).
- `HINT_MS` (3500) / `RELEASE_DB_MS` (30) in `src/challenge.cpp`.
- `FREQ` (700) in `src/morse.cpp` — sidetone/playback tone.
- Word lists per language/level — `src/i18n.cpp` (`WORDS_*`, sorted by
  difficulty; `levelWords()` filters by the current level's letter cap).

## Layout of the code

| File | Role |
|------|------|
| `pins.h`, `es8311.*`, `touch.*`, `sys.*` | hardware layer (shared with `arkade`) |
| `display.*` | LCD init, centered text, palette |
| `audio.*` | ES8311 tones **+ live sidetone** (`feedKey`/`keyOff`) |
| `morse.*` | ITU A–Z table, learning order, sound playback |
| `i18n.*` | EN/PT/ES strings and word lists |
| `ui.*` | buttons, dot/dash glyphs, the KEY button |
| `challenge.*` | shared keyer: input + hint + evaluation |
| `tutorial.cpp`, `game_letters.cpp`, `game_words.cpp`, `game_listen.cpp` | the four modes |
| `main.cpp` | menu + settings + dispatch |

> This is a proof of concept. Next step (already sketched in *Listen*): expand
> the reverse mode and tune the difficulty curve after testing on-device.
