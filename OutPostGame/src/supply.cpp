#include "supply.h"
#include "config.h"
#include "st7789.h"
#include "character.h"

namespace {

static const int SUPPLY_MERCHANT_X = 6;
static const int SUPPLY_MERCHANT_Y = 5;
static const int SUPPLY_SCOUT_X = 2;
static const int SUPPLY_SCOUT_Y = 5;
static const int SUPPLY_CIVILIAN_X = 10;
static const int SUPPLY_CIVILIAN_Y = 5;
static const int SUPPLY_WIZARD_X = 10;
static const int SUPPLY_WIZARD_Y = 8;

static void drawLantern(int px, int py) {
    lcd.drawFastVLine(px + 7, py + 4, 7, 0x9CD3);
    lcd.fillRect(px + 5, py + 2, 6, 3, 0xFDA0);
    lcd.fillRect(px + 6, py + 3, 4, 2, COLOR_YELLOW);
    lcd.drawPixel(px + 7, py + 1, COLOR_WHITE);
    lcd.drawFastHLine(px + 4, py + 11, 8, 0x734D);
}

static void drawMarketRug(int px, int py, uint16_t mainColor, uint16_t trimColor) {
    lcd.fillRect(px + 2, py + 6, 12, 6, mainColor);
    lcd.drawRect(px + 2, py + 6, 12, 6, trimColor);
    lcd.drawFastHLine(px + 4, py + 8, 8, trimColor);
    lcd.drawFastHLine(px + 4, py + 10, 8, trimColor);
}

static void drawCratePile(int px, int py, bool tall) {
    uint16_t wood = 0x8A85;
    uint16_t woodDark = 0x5A03;

    lcd.fillRect(px + 2, py + 8, 12, 5, wood);
    lcd.drawRect(px + 2, py + 8, 12, 5, woodDark);
    lcd.drawFastVLine(px + 6, py + 8, 5, woodDark);
    lcd.drawFastVLine(px + 10, py + 8, 5, woodDark);
    if (tall) {
        lcd.fillRect(px + 4, py + 4, 8, 4, wood);
        lcd.drawRect(px + 4, py + 4, 8, 4, woodDark);
    } else {
        lcd.fillRect(px + 5, py + 5, 6, 3, 0xFDA0);
    }
}

static void drawTradeHallTop(int px, int py, int tx) {
    uint16_t roof = 0x52B2;
    uint16_t roofDark = 0x31AE;

    lcd.fillRect(px, py + 3, 16, 7, roof);
    lcd.drawFastHLine(px, py + 3, 16, COLOR_WHITE);
    lcd.drawFastHLine(px, py + 9, 16, roofDark);
    if (tx >= 4 && tx <= 8) {
        lcd.fillRect(px, py + 4, 16, 4, 0x5A03);
        lcd.drawFastHLine(px, py + 4, 16, 0xFDE0);
        lcd.drawFastHLine(px, py + 7, 16, 0x8A84);
        switch (tx) {
            case 4: lcd.drawText(px + 5, py + 4, "T", COLOR_YELLOW, 0x5A03, 1); break;
            case 5: lcd.drawText(px + 5, py + 4, "R", COLOR_YELLOW, 0x5A03, 1); break;
            case 6: lcd.drawText(px + 5, py + 4, "A", COLOR_YELLOW, 0x5A03, 1); break;
            case 7: lcd.drawText(px + 5, py + 4, "D", COLOR_YELLOW, 0x5A03, 1); break;
            case 8: lcd.drawText(px + 5, py + 4, "E", COLOR_YELLOW, 0x5A03, 1); break;
        }
    }
}

static void drawTradeHallBody(int px, int py, bool leftEdge, bool rightEdge) {
    uint16_t wall = 0xC618;
    uint16_t shade = 0x8C71;
    uint16_t beam = 0x5A03;

    lcd.fillRect(px + 1, py, 14, 16, wall);
    lcd.drawFastHLine(px + 1, py, 14, COLOR_WHITE);
    lcd.drawFastHLine(px + 1, py + 15, 14, shade);
    if (leftEdge)  lcd.fillRect(px + 1, py + 2, 3, 12, beam);
    if (rightEdge) lcd.fillRect(px + 12, py + 2, 3, 12, beam);
    lcd.fillRect(px + 5, py + 4, 6, 6, 0x6B4D);
    lcd.drawRect(px + 5, py + 4, 6, 6, 0xFDE0);
    lcd.drawPixel(px + 7, py + 6, COLOR_YELLOW);
    lcd.drawPixel(px + 8, py + 6, COLOR_YELLOW);
}

static void drawTradeCounter(int px, int py, bool leftEdge, bool rightEdge) {
    uint16_t counter = 0x8A85;
    uint16_t top = 0xD6BA;
    uint16_t post = 0x5A03;

    lcd.fillRect(px, py + 8, 16, 6, counter);
    lcd.drawFastHLine(px + 1, py + 7, 14, top);
    lcd.drawFastHLine(px + 1, py + 13, 14, 0x4A49);
    if (leftEdge)  lcd.fillRect(px + 1, py + 4, 3, 10, post);
    if (rightEdge) lcd.fillRect(px + 12, py + 4, 3, 10, post);
    lcd.fillRect(px + 5, py + 9, 6, 3, 0x39CE);
    lcd.fillRect(px + 6, py + 10, 4, 2, COLOR_CYAN);
}

static void drawSideStall(int px, int py, bool leftSide, bool lowerTile) {
    uint16_t canopy = leftSide ? 0xC1A4 : 0x03EF;
    uint16_t wood = 0x8A85;
    uint16_t woodDark = 0x5A03;

    if (!lowerTile) {
        lcd.fillRect(px + 1, py + 5, 14, 4, canopy);
        lcd.drawFastHLine(px + 1, py + 5, 14, COLOR_WHITE);
        lcd.drawFastHLine(px + 1, py + 8, 14, woodDark);
        lcd.fillRect(px + 2, py + 9, 12, 3, wood);
        if (leftSide) lcd.drawText(px + 4, py + 10, "K", COLOR_YELLOW, wood, 1);
        else          lcd.drawText(px + 4, py + 10, "S", COLOR_YELLOW, wood, 1);
    } else {
        lcd.fillRect(px + 2, py, 12, 11, wood);
        lcd.drawRect(px + 2, py, 12, 11, woodDark);
        lcd.drawFastVLine(px + 4, py + 2, 9, woodDark);
        lcd.drawFastVLine(px + 11, py + 2, 9, woodDark);
        lcd.fillRect(px + 5, py + 4, 6, 4, 0x39CE);
        lcd.drawRect(px + 5, py + 4, 6, 4, 0xD6BA);
        if (leftSide) {
            lcd.drawPixel(px + 7, py + 6, COLOR_ORANGE);
        } else {
            lcd.fillRect(px + 6, py + 6, 4, 2, COLOR_CYAN);
        }
    }
}

}  // namespace

void supplyDrawGround(int px, int py, int tx, int ty) {
    lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x4A48);
    lcd.drawFastHLine(px, py + 15, TILE_SIZE, 0x39A6);
    lcd.drawFastVLine(px + 15, py, TILE_SIZE, 0x39A6);

    if (((tx + ty) & 1) == 0) {
        lcd.drawFastHLine(px + 1, py + 5, 6, 0x736D);
        lcd.drawFastHLine(px + 8, py + 10, 5, 0x736D);
    }
    if (((tx * 5 + ty * 3) & 3) == 1) {
        lcd.drawPixel(px + 4, py + 11, 0xBDF7);
        lcd.drawPixel(px + 11, py + 4, 0xBDF7);
    }
}

void supplyDrawDecor(int px, int py, int tx, int ty) {
    supplyDrawGround(px, py, tx, ty);

    if (tx == SUPPLY_MERCHANT_X && ty == SUPPLY_MERCHANT_Y) {
        characterDrawMerchant(px, py);
        return;
    }

    if (tx == SUPPLY_SCOUT_X && ty == SUPPLY_SCOUT_Y) {
        characterDrawRedVisorScout(px, py);
        return;
    }

    if (tx == SUPPLY_CIVILIAN_X && ty == SUPPLY_CIVILIAN_Y) {
        characterDrawCivilian(px, py);
        return;
    }

    if (tx == SUPPLY_WIZARD_X && ty == SUPPLY_WIZARD_Y) {
        characterDrawWizard(px, py);
        return;
    }

    if ((tx == 2 && ty == 2) || (tx == 11 && ty == 2)) {
        drawLantern(px, py);
        return;
    }

    if (tx == 4 && ty == 5) {
        drawMarketRug(px, py, 0xA145, 0xFDE0);
        return;
    }

    if ((tx == 3 && ty == 9) || (tx == 10 && ty == 9)) {
        drawCratePile(px, py, false);
        return;
    }

    if ((tx == 1 && ty == 7) || (tx == 12 && ty == 7)) {
        drawMarketRug(px, py, 0x528A, 0xC618);
        return;
    }

    lcd.drawFastHLine(px + 4, py + 9, 8, 0x8C71);
    lcd.drawFastVLine(px + 8, py + 6, 4, 0x6B4D);
    lcd.drawPixel(px + 7, py + 8, COLOR_YELLOW);
}

void supplyDrawBlocker(int px, int py, int tx, int ty) {
    supplyDrawGround(px, py, tx, ty);

    if (tx >= 4 && tx <= 9 && ty == 2) {
        drawTradeHallTop(px, py, tx);
        return;
    }

    if (tx >= 4 && tx <= 9 && ty == 3) {
        drawTradeHallBody(px, py, tx == 4, tx == 9);
        return;
    }

    if (tx == 4 && ty == 4) {
        drawTradeCounter(px, py, true, false);
        return;
    }
    if (tx == 9 && ty == 4) {
        drawTradeCounter(px, py, false, true);
        return;
    }

    if ((tx >= 4 && tx <= 5 && ty >= 7 && ty <= 8) ||
        (tx >= 8 && tx <= 9 && ty >= 7 && ty <= 8)) {
        drawCratePile(px, py, true);
        if (ty == 7) {
            lcd.fillRect(px + 3, py + 2, 10, 3, 0xD6BA);
            lcd.drawFastHLine(px + 4, py + 3, 8, 0xFDE0);
        }
        return;
    }

    if (tx >= 1 && tx <= 2 && ty >= 8 && ty <= 9) {
        drawSideStall(px, py, true, ty == 9);
        return;
    }

    if (tx >= 11 && tx <= 12 && ty >= 8 && ty <= 9) {
        drawSideStall(px, py, false, ty == 9);
        return;
    }

    lcd.fillRect(px + 3, py + 6, 10, 6, 0x8A85);
    lcd.drawRect(px + 3, py + 6, 10, 6, 0x5A03);
}

bool supplyNearWarrior(int playerX, int playerY) {
    return abs(playerX - SUPPLY_SCOUT_X) <= 1 &&
           abs(playerY - SUPPLY_SCOUT_Y) <= 1;
}

bool supplyNearMerchant(int playerX, int playerY) {
    return abs(playerX - SUPPLY_MERCHANT_X) <= 1 &&
           abs(playerY - SUPPLY_MERCHANT_Y) <= 1;
}

bool supplyNearCivilian(int playerX, int playerY) {
    return abs(playerX - SUPPLY_CIVILIAN_X) <= 1 &&
           abs(playerY - SUPPLY_CIVILIAN_Y) <= 1;
}

bool supplyNearWizard(int playerX, int playerY) {
    return abs(playerX - SUPPLY_WIZARD_X) <= 1 &&
           abs(playerY - SUPPLY_WIZARD_Y) <= 1;
}

bool supplyIsSolidEntity(int tileX, int tileY) {
    return (tileX == SUPPLY_MERCHANT_X  && tileY == SUPPLY_MERCHANT_Y) ||
           (tileX == SUPPLY_SCOUT_X     && tileY == SUPPLY_SCOUT_Y) ||
           (tileX == SUPPLY_CIVILIAN_X  && tileY == SUPPLY_CIVILIAN_Y) ||
           (tileX == SUPPLY_WIZARD_X    && tileY == SUPPLY_WIZARD_Y);
}
