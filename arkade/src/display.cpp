// ===========================================================================
//  display.cpp — Toda a parte gráfica.
// ===========================================================================
#include "display.h"
#include "pins.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

namespace display {

// --- Geometria da tela ----------------------------------------------------
static const int SCREEN = 240;
static const int STATUS_H = 40;              // faixa de status no topo
static const int GX = 20, GY = 42;           // canto superior esq. da grade
static const int CELL = 65;                  // lado de cada célula
static const int GRID = CELL * 3;            // 195

// --- Objetos Arduino_GFX ---------------------------------------------------
static Arduino_DataBus* bus = nullptr;
static Arduino_GFX*     gfx = nullptr;

// --- Paleta (todas públicas) ----------------------------------------------
uint16_t COL_BG, COL_GRID, COL_X, COL_O, COL_TEXT, COL_WIN, COL_DRAW;

static void centerText(const char* s, int cx, int cy, uint8_t size, uint16_t color); // fwd

Arduino_GFX* screen() { return gfx; }
void text(const char* s, int cx, int cy, uint8_t size, uint16_t color) {
    centerText(s, cx, cy, size, color);
}

// --- Botões do menu (x, y, w, h) ------------------------------------------
struct Rect { int x, y, w, h; };
static const int NBTN = 4;
static const Rect BTN[NBTN] = {
    { 20,  52, 200, 40 },
    { 20, 100, 200, 40 },
    { 20, 148, 200, 40 },
    { 20, 196, 200, 40 },
};
static const char* BTN_LABEL[NBTN] = { "2 Jogadores", "IA - Facil", "IA - Media", "IA - Dificil" };

// --- Helpers --------------------------------------------------------------

// Texto centralizado em (cx,cy) usando a fonte clássica 6x8.
static void centerText(const char* s, int cx, int cy, uint8_t size, uint16_t color) {
    int w = (int)strlen(s) * 6 * size;
    int h = 8 * size;
    gfx->setTextColor(color);
    gfx->setTextSize(size);
    gfx->setCursor(cx - w / 2, cy - h / 2);
    gfx->print(s);
}

// Linha grossa (aproximada) desenhando várias paralelas deslocadas.
static void thickLine(int x0, int y0, int x1, int y1, int t, uint16_t c) {
    int half = t / 2;
    for (int o = -half; o <= half; o++) {
        gfx->drawLine(x0 + o, y0, x1 + o, y1, c);
        gfx->drawLine(x0, y0 + o, x1, y1 + o, c);
    }
}

// Anel grosso (várias circunferências concêntricas).
static void thickCircle(int cx, int cy, int r, int t, uint16_t c) {
    for (int i = 0; i < t; i++) gfx->drawCircle(cx, cy, r - i, c);
}

static int cellCenterX(int idx) { return GX + (idx % 3) * CELL + CELL / 2; }
static int cellCenterY(int idx) { return GY + (idx / 3) * CELL + CELL / 2; }

// --- API ------------------------------------------------------------------

void begin() {
    bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI, LCD_MISO);
    gfx = new Arduino_ST7789(bus, LCD_RST, 0 /*rotação*/, true /*IPS*/,
                             LCD_W, LCD_H);
    gfx->begin();

    // Backlight ligado.
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);

    // Paleta (RGB565).
    COL_BG   = gfx->color565(16, 18, 27);     // fundo escuro
    COL_GRID = gfx->color565(70, 78, 100);    // linhas da grade
    COL_X    = gfx->color565(255, 95, 85);    // X vermelho/coral
    COL_O    = gfx->color565(70, 200, 235);   // O ciano
    COL_TEXT = gfx->color565(235, 238, 245);  // texto claro
    COL_WIN  = gfx->color565(120, 230, 140);  // verde de vitória
    COL_DRAW = gfx->color565(240, 200, 90);   // amarelo de empate

    gfx->fillScreen(COL_BG);
}

void drawMenu() {
    gfx->fillScreen(COL_BG);
    centerText("JOGO DA VELHA", SCREEN / 2, 26, 2, COL_TEXT);

    // Cor por dificuldade: branco (2P), ciano (fácil), amarelo (média), vermelho (difícil).
    uint16_t accent[NBTN] = { COL_TEXT, COL_O, COL_DRAW, COL_X };
    for (int i = 0; i < NBTN; i++) {
        const Rect& b = BTN[i];
        gfx->drawRoundRect(b.x, b.y, b.w, b.h, 8, accent[i]);
        gfx->drawRoundRect(b.x + 1, b.y + 1, b.w - 2, b.h - 2, 8, accent[i]);
        centerText(BTN_LABEL[i], b.x + b.w / 2, b.y + b.h / 2, 2, accent[i]);
    }
}

int menuHit(uint16_t x, uint16_t y) {
    for (int i = 0; i < NBTN; i++) {
        const Rect& b = BTN[i];
        if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) return i;
    }
    return -1;
}

void drawBoardFrame() {
    gfx->fillScreen(COL_BG);
    // Linhas da grade (grossas).
    for (int i = 1; i <= 2; i++) {
        int gx = GX + i * CELL;
        int gy = GY + i * CELL;
        gfx->fillRect(gx - 1, GY, 3, GRID, COL_GRID);        // verticais
        gfx->fillRect(GX, gy - 1, GRID, 3, COL_GRID);        // horizontais
    }
}

// Desenha o X/O com um fator de escala (1.0 = tamanho cheio).
static void drawMarkScaled(int idx, Cell who, float scale) {
    int cx = cellCenterX(idx), cy = cellCenterY(idx);
    int r = (int)((CELL / 2 - 12) * scale);
    if (r < 2) return;
    if (who == X) {
        thickLine(cx - r, cy - r, cx + r, cy + r, 5, COL_X);
        thickLine(cx - r, cy + r, cx + r, cy - r, 5, COL_X);
    } else if (who == O) {
        thickCircle(cx, cy, r, 4, COL_O);
    }
}

void drawMark(int idx, Cell who) {
    drawMarkScaled(idx, who, 1.0f);
}

void animateMark(int idx, Cell who) {
    const int STEPS = 5;
    int cx = cellCenterX(idx), cy = cellCenterY(idx);
    for (int i = 1; i <= STEPS; i++) {
        // Limpa só o interior da célula (sem tocar nas linhas da grade).
        gfx->fillRect(cx - CELL / 2 + 3, cy - CELL / 2 + 3, CELL - 6, CELL - 6, COL_BG);
        drawMarkScaled(idx, who, (float)i / STEPS);
        delay(18);
    }
}

void drawStatus(const char* text, uint16_t color) {
    gfx->fillRect(0, 0, SCREEN, STATUS_H, COL_BG);
    centerText(text, SCREEN / 2, STATUS_H / 2, 2, color);
}

void drawFooter(const char* text, uint16_t color) {
    gfx->fillRect(0, SCREEN - 18, SCREEN, 18, COL_BG);
    centerText(text, SCREEN / 2, SCREEN - 9, 1, color);
}

void drawWinLine(int8_t line) {
    if (line < 0 || line > 7) return;
    // Célula inicial e final de cada uma das 8 linhas vencedoras.
    static const uint8_t ENDS[8][2] = {
        {0, 2}, {3, 5}, {6, 8},   // horizontais
        {0, 6}, {1, 7}, {2, 8},   // verticais
        {0, 8}, {2, 6},           // diagonais
    };
    int a = ENDS[line][0], b = ENDS[line][1];
    thickLine(cellCenterX(a), cellCenterY(a),
              cellCenterX(b), cellCenterY(b), 7, COL_WIN);
}

int cellFromTouch(uint16_t x, uint16_t y) {
    if (x < GX || x >= GX + GRID) return -1;
    if (y < GY || y >= GY + GRID) return -1;
    int col = (x - GX) / CELL;
    int row = (y - GY) / CELL;
    if (col > 2 || row > 2) return -1;
    return row * 3 + col;
}

void drawResultScreen(const char* headline, uint16_t hcol,
                      int sx, int so, int draws, bool aiMode) {
    gfx->fillScreen(COL_BG);
    centerText(headline, SCREEN / 2, 42, 3, hcol);          // resultado grande

    centerText("PLACAR", SCREEN / 2, 96, 2, COL_TEXT);
    char line[40];
    if (aiMode) snprintf(line, sizeof line, "Voce %d   IA %d", sx, so);
    else        snprintf(line, sizeof line, "X %d   O %d", sx, so);
    centerText(line, SCREEN / 2, 128, 2, COL_TEXT);
    snprintf(line, sizeof line, "Empates %d", draws);
    centerText(line, SCREEN / 2, 156, 2, COL_DRAW);

    centerText("toque: menu", SCREEN / 2, 200, 1, COL_TEXT);
    centerText("chacoalhe: de novo", SCREEN / 2, 216, 1, COL_TEXT);
}

void drawSleepScreen() {
    gfx->fillScreen(COL_BG);
    centerText("ate mais!", SCREEN / 2, SCREEN / 2 - 14, 3, COL_O);
    centerText("PWR para ligar", SCREEN / 2, SCREEN / 2 + 26, 1, COL_TEXT);
}

void drawSplash(const char* title, const char* sub) {
    gfx->fillScreen(COL_BG);
    centerText("BOOT", SCREEN / 2, 70, 2, COL_TEXT);
    centerText(title, SCREEN / 2, 112, 3, COL_O);       // motivo do reset
    centerText(sub, SCREEN / 2, 152, 2, COL_DRAW);      // causa do wake
}

void drawBattery(int pct, bool charging) {
    if (pct < 0) pct = 0; if (pct > 100) pct = 100;
    const int w = 24, h = 12;
    const int x = SCREEN - w - 8;   // canto superior direito
    const int y = 5;

    // Corpo + terminal da bateria.
    gfx->fillRect(x - 40, 0, 40 + w + 6, h + 6, COL_BG);   // limpa a área antes
    gfx->drawRect(x, y, w, h, COL_TEXT);
    gfx->fillRect(x + w, y + 3, 2, h - 6, COL_TEXT);

    // Preenchimento proporcional, com cor por nível.
    uint16_t c = (pct > 40) ? COL_WIN : (pct > 20) ? COL_DRAW : COL_X;
    int fillW = (w - 4) * pct / 100;
    if (fillW > 0) gfx->fillRect(x + 2, y + 2, fillW, h - 4, c);

    // Percentual à esquerda do ícone (e "+" se carregando).
    char buf[8];
    snprintf(buf, sizeof buf, "%d%%", pct);
    int tw = (int)strlen(buf) * 6;
    gfx->setTextSize(1);
    gfx->setTextColor(charging ? COL_O : COL_TEXT);
    gfx->setCursor(x - tw - 4, y + 3);
    gfx->print(buf);
    if (charging) {
        gfx->setTextColor(COL_O);
        gfx->setCursor(x - tw - 12, y + 3);
        gfx->print("+");
    }
}

} // namespace display
