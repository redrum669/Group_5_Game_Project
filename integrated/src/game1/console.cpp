#include "game1/scenes/console.h"
#include "config.h"
#include "st7789.h"
#include "game1/entities/character.h"
#include "game1/entities/weapon.h"

static void drawMenuSpriteLayer(int px, int py, const uint8_t* data, uint16_t color) {
    for (int row = 0; row < 8; row++) {
        uint8_t rowBits = data[row];
        for (int col = 0; col < 8; col++) {
            if ((rowBits >> (7 - col)) & 1) {
                lcd.fillRect(px + col * 2, py + row * 2, 2, 2, color);
            }
        }
    }
}

static void drawMenuCircle(int cx, int cy, int r, uint16_t color) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    while (x <= y) {
        lcd.drawPixel(cx + x, cy + y, color);
        lcd.drawPixel(cx - x, cy + y, color);
        lcd.drawPixel(cx + x, cy - y, color);
        lcd.drawPixel(cx - x, cy - y, color);
        lcd.drawPixel(cx + y, cy + x, color);
        lcd.drawPixel(cx - y, cy + x, color);
        lcd.drawPixel(cx + y, cy - x, color);
        lcd.drawPixel(cx - y, cy - x, color);
        if (d < 0) d += 4 * x + 6;
        else {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

static void drawCowSprite(int px, int py, bool facingLeft) {
    static const uint8_t body[8] = {
        0x00, 0x7E, 0xFF, 0xFF, 0xFF, 0x7E, 0x24, 0x24
    };
    static const uint8_t spotA[8] = {
        0x00, 0x00, 0x10, 0x18, 0x08, 0x10, 0x00, 0x00
    };
    static const uint8_t spotB[8] = {
        0x00, 0x00, 0x06, 0x0E, 0x04, 0x08, 0x00, 0x00
    };
    static const uint8_t headLeft[8] = {
        0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xE0, 0xC0, 0x00
    };
    static const uint8_t headRight[8] = {
        0x00, 0x00, 0x03, 0x07, 0x0F, 0x07, 0x03, 0x00
    };
    static const uint8_t hornLeft[8] = {
        0x00, 0x80, 0x40, 0x00, 0x80, 0x00, 0x00, 0x00
    };
    static const uint8_t hornRight[8] = {
        0x00, 0x01, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00
    };

    drawMenuSpriteLayer(px, py, body, COLOR_WHITE);
    drawMenuSpriteLayer(px, py, spotA, COLOR_BLACK);
    drawMenuSpriteLayer(px, py, spotB, COLOR_BLACK);
    if (facingLeft) {
        drawMenuSpriteLayer(px, py, hornLeft, 0xC618);
        drawMenuSpriteLayer(px, py, headLeft, COLOR_WHITE);
        lcd.drawPixel(px + 1, py + 8, COLOR_BLACK);
    } else {
        drawMenuSpriteLayer(px, py, hornRight, 0xC618);
        drawMenuSpriteLayer(px, py, headRight, COLOR_WHITE);
        lcd.drawPixel(px + 14, py + 8, COLOR_BLACK);
    }
}

static void drawMenuBorder() {
    lcd.fillScreen(0x0847);
    lcd.drawRect(6, 6, 228, 228, 0x39CE);
    lcd.drawRect(8, 8, 224, 224, 0x632C);

    for (int i = 0; i < 8; i++) {
        int x = 18 + (i * 23) % 190;
        int y = 92 + (i * 13) % 18;
        lcd.fillRect(x, y, 2, 2, 0x632C);
    }
}

static void drawMenuCgShip(int x, int y) {
    drawMenuCircle(x, y, 10, COLOR_GOLD);
    drawMenuCircle(x, y, 11, 0x8400);
    lcd.fillRect(x - 14, y - 3, 28, 8, COLOR_DARKGRAY);
    lcd.fillRect(x - 9,  y - 6, 18, 5, COLOR_GRAY);
    lcd.fillRect(x - 4,  y - 9,  8, 4, COLOR_LIGHTGRAY);
    lcd.fillRect(x - 4,  y - 6,  8, 5, COLOR_CYAN);
    drawMenuCircle(x, y - 4, 5, COLOR_WHITE);
    lcd.fillRect(x - 11, y + 3, 3, 2, COLOR_ORANGE);
    lcd.fillRect(x - 3,  y + 3, 3, 2, COLOR_ORANGE);
    lcd.fillRect(x + 4,  y + 3, 3, 2, COLOR_ORANGE);
    lcd.fillRect(x + 9,  y + 3, 3, 2, COLOR_ORANGE);
}

static void drawMenuAlienCreature(int px, int py) {
    static const uint8_t body[8]  = {0x00, 0x3C, 0x7E, 0xFF, 0xBD, 0xFF, 0x66, 0x42};
    static const uint8_t eyes[8]  = {0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t claws[8] = {0x00, 0x00, 0x00, 0x81, 0x42, 0x81, 0x00, 0x00};

    drawMenuSpriteLayer(px, py, body,  0x7BE0);
    drawMenuSpriteLayer(px, py, eyes,  0xF81F);
    drawMenuSpriteLayer(px, py, claws, 0xFFFF);
}

static void drawExplorationCard(int x, int y) {
    lcd.fillRect(x, y, 88, 118, 0x2108);
    lcd.drawRect(x, y, 88, 118, COLOR_YELLOW);
    lcd.drawRect(x + 1, y + 1, 86, 116, COLOR_YELLOW);

    lcd.fillRect(x + 4, y + 4, 80, 86, 0x296B);
    for (int py = 0; py < 36; py++) {
        uint16_t c = (py < 18) ? 0x18C8 : 0x31AA;
        lcd.drawFastHLine(x + 4, y + 4 + py, 80, c);
    }
    drawMenuCgShip(x + 61, y + 16);
    lcd.fillRect(x + 58, y + 22, 6, 18, 0x07E0);
    lcd.fillRect(x + 57, y + 39, 8, 6, 0x17E0);
    lcd.fillRect(x + 14, y + 48, 10, 16, 0x52AE);
    lcd.fillRect(x + 24, y + 42, 8, 22, 0x4208);
    lcd.fillRect(x + 62, y + 50, 7, 16, 0x4208);
    lcd.drawFastHLine(x + 4, y + 64, 80, 0x6ACA);
    lcd.fillRect(x + 4, y + 66, 80, 20, 0x4221);
    lcd.fillRect(x + 8, y + 74, 18, 6, 0x528A);
    lcd.fillRect(x + 68, y + 72, 10, 8, 0x528A);

    characterDraw(x + 25, y + 48, WEAPON_SMG);
    drawMenuAlienCreature(x + 52, y + 58);

    lcd.fillRect(x + 11, y + 94, 66, 14, 0x18C6);
    lcd.drawRect(x + 11, y + 94, 66, 14, COLOR_YELLOW);
    lcd.drawText(x + 15, y + 98, "Exploration", COLOR_WHITE, 0x18C6, 1);
}

static void drawFarmingCard(int x, int y) {
    lcd.fillRect(x, y, 88, 118, 0x2108);
    lcd.drawRect(x, y, 88, 118, COLOR_YELLOW);
    lcd.drawRect(x + 1, y + 1, 86, 116, COLOR_YELLOW);

    lcd.fillRect(x + 4, y + 4, 80, 86, 0x426D);
    for (int py = 0; py < 36; py++) {
        uint16_t c = (py < 16) ? 0x4A8E : 0x73CF;
        lcd.drawFastHLine(x + 4, y + 4 + py, 80, c);
    }
    lcd.fillRect(x + 4, y + 62, 80, 24, 0x8A84);
    for (int row = 0; row < 3; row++) {
        lcd.drawFastHLine(x + 4, y + 66 + row * 7, 80, 0x6A03);
    }
    for (int i = 0; i < 6; i++) {
        int cx = x + 10 + i * 12;
        lcd.drawFastVLine(cx, y + 64 + (i & 1), 18, 0x03E0);
        lcd.drawFastVLine(cx + 2, y + 66 + ((i + 1) & 1), 14, 0x07E0);
        lcd.drawPixel(cx - 1, y + 80, 0x05A0);
    }
    lcd.fillRect(x + 56, y + 50, 18, 14, 0xC638);
    lcd.drawFastHLine(x + 62, y + 43, 6, 0xA2A0);
    lcd.drawFastHLine(x + 60, y + 44, 10, 0xA2A0);
    lcd.drawFastHLine(x + 58, y + 45, 14, 0xA2A0);
    lcd.drawFastHLine(x + 56, y + 46, 18, 0xA2A0);
    lcd.drawFastHLine(x + 55, y + 47, 20, 0xA2A0);
    lcd.drawFastHLine(x + 55, y + 48, 20, 0xA2A0);
    lcd.drawFastHLine(x + 55, y + 49, 20, 0xA2A0);
    lcd.drawFastVLine(x + 64, y + 56, 8, 0x62E0);
    lcd.fillRect(x + 58, y + 54, 4, 4, 0x7D9F);
    lcd.fillRect(x + 68, y + 54, 4, 4, 0x7D9F);
    lcd.drawRect(x + 56, y + 50, 18, 14, 0x6B4D);

    drawCowSprite(x + 10, y + 46, false);
    characterDraw(x + 37, y + 41, WEAPON_SMG);

    lcd.fillRect(x + 17, y + 94, 54, 14, 0x18C6);
    lcd.drawRect(x + 17, y + 94, 54, 14, COLOR_YELLOW);
    lcd.drawText(x + 24, y + 98, "Farming", COLOR_WHITE, 0x18C6, 1);
}

void consoleDrawMainMenu() {
    drawMenuBorder();

    lcd.drawText(48, 28, "OUTPOST OF THE LAST DAWN", COLOR_WHITE, 0x0847, 1);
    lcd.drawFastHLine(32, 50, 176, 0x632C);

    drawExplorationCard(20, 94);
    drawFarmingCard(132, 94);

    lcd.drawText(22, 214, "A: START EXPLORATION", COLOR_YELLOW, 0x0847, 1);
    lcd.drawText(67, 226, "B: FARMING", COLOR_GREEN, 0x0847, 1);
}
