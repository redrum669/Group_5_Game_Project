#include "altar.h"
#include "config.h"
#include "st7789.h"
#include "character.h"

// Static position definitions for altar scene elements
static const int ALTAR_PRIEST_X = 4;
static const int ALTAR_PRIEST_Y = 5;
static const int ALTAR_SKULL_X = 9;
static const int ALTAR_SKULL_Y = 5;
static const int ALTAR_OFFER_X = 6;
static const int ALTAR_OFFER_Y = 7;

// Draw tall warp plant with leaning direction
static void drawWarpPlantTall(int px, int py, bool leanLeft) {
    uint16_t stem = 0x39C7;
    uint16_t bulb = 0xA81F;
    uint16_t glow = 0xF81F;

    if (leanLeft) {
        lcd.drawFastVLine(px + 9, py + 6, 6, stem);
        lcd.drawFastVLine(px + 8, py + 4, 4, stem);
        lcd.drawFastHLine(px + 5, py + 5, 4, stem);
        lcd.fillRect(px + 4, py + 3, 4, 3, bulb);
        lcd.drawPixel(px + 5, py + 2, glow);
        lcd.drawPixel(px + 6, py + 2, glow);
        lcd.drawPixel(px + 3, py + 4, 0x600C);
    } else {
        lcd.drawFastVLine(px + 6, py + 6, 6, stem);
        lcd.drawFastVLine(px + 7, py + 4, 4, stem);
        lcd.drawFastHLine(px + 7, py + 5, 4, stem);
        lcd.fillRect(px + 8, py + 3, 4, 3, bulb);
        lcd.drawPixel(px + 9, py + 2, glow);
        lcd.drawPixel(px + 10, py + 2, glow);
        lcd.drawPixel(px + 12, py + 4, 0x600C);
    }
}

// Draw low-profile warp plant decoration
static void drawWarpPlantLow(int px, int py) {
    lcd.drawFastHLine(px + 4, py + 10, 8, 0x39C7);
    lcd.drawFastVLine(px + 7, py + 7, 4, 0x39C7);
    lcd.drawFastVLine(px + 10, py + 6, 5, 0x39C7);
    lcd.fillRect(px + 5, py + 8, 3, 2, 0xA81F);
    lcd.fillRect(px + 9, py + 7, 3, 2, 0xA81F);
    lcd.drawPixel(px + 6, py + 7, 0xF81F);
    lcd.drawPixel(px + 10, py + 6, 0xF81F);
}

// Draw small rune shard for ground decoration
static void drawRuneShard(int px, int py) {
    lcd.drawFastVLine(px + 8, py + 4, 6, 0xB81F);
    lcd.drawFastHLine(px + 6, py + 7, 5, 0x8010);
    lcd.drawPixel(px + 8, py + 3, 0xF81F);
    lcd.drawPixel(px + 7, py + 11, 0xC618);
}

// Draw offering altar platform with visual state
static void drawOfferAltar(int px, int py, int tx) {
    lcd.fillRect(px + 1, py + 8, 14, 6, 0x39CE);
    lcd.drawFastHLine(px + 2, py + 7, 12, 0xC618);
    lcd.drawFastHLine(px + 3, py + 8, 10, 0x9B15);
    
    if (tx == ALTAR_OFFER_X) {
        lcd.fillRect(px + 8, py + 5, 8, 4, 0x600C);
        lcd.fillRect(px + 10, py + 6, 6, 2, 0xA81F);
        lcd.drawFastVLine(px + 6, py + 10, 3, 0x2108);
    } else {
        lcd.fillRect(px, py + 5, 8, 4, 0x600C);
        lcd.fillRect(px, py + 6, 6, 2, 0xA81F);
        lcd.drawFastVLine(px + 9, py + 10, 3, 0x2108);
    }
    
    lcd.drawPixel(px + 7, py + 6, 0xF81F);
    lcd.drawPixel(px + 8, py + 6, 0xF81F);
}

// Render base ground texture for altar tiles
void altarDrawGround(int px, int py, int tx, int ty) {
    lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x0804);
    lcd.drawFastHLine(px, py + 15, TILE_SIZE, 0x1006);
    lcd.drawFastVLine(px + 15, py, TILE_SIZE, 0x1006);

    if (((tx * 3 + ty * 5) & 1) == 0) {
        lcd.drawFastHLine(px + 2, py + 4, 5, 0x500A);
        lcd.drawPixel(px + 7, py + 5, 0x8010);
    }
    if (((tx * 7 + ty * 3) & 3) == 1) {
        lcd.drawFastVLine(px + 11, py + 6, 4, 0x500A);
        lcd.drawPixel(px + 10, py + 8, 0xA81F);
    }
    if (((tx + ty * 2) & 3) == 2) {
        lcd.drawPixel(px + 4, py + 11, 0x39C7);
        lcd.drawPixel(px + 5, py + 10, 0x600C);
    }
}

// Render decorative elements over altar ground tiles
void altarDrawDecor(int px, int py, int tx, int ty) {
    altarDrawGround(px, py, tx, ty);

    if (tx == ALTAR_PRIEST_X && ty == ALTAR_PRIEST_Y) {
        characterDrawPriest(px, py);
        return;
    }

    if (tx == ALTAR_SKULL_X && ty == ALTAR_SKULL_Y) {
        characterDrawSkull(px, py);
        return;
    }

    if ((tx == 4 && ty == 1) || (tx == 10 && ty == 2) ||
        (tx == 3 && ty == 6) || (tx == 11 && ty == 8)) {
        drawWarpPlantTall(px, py, true);
        return;
    }

    if ((tx == 9 && ty == 1) || (tx == 2 && ty == 3) ||
        (tx == 10 && ty == 6) || (tx == 5 && ty == 9)) {
        drawWarpPlantTall(px, py, false);
        return;
    }

    if ((tx == 6 && ty == 2) || (tx == 5 && ty == 4) ||
        (tx == 9 && ty == 7) || (tx == 8 && ty == 8)) {
        drawWarpPlantLow(px, py);
        return;
    }

    drawRuneShard(px, py);
}

// Render collision blocks and altar structures
void altarDrawBlocker(int px, int py, int tx, int ty) {
    altarDrawGround(px, py, tx, ty);

    if ((tx == ALTAR_OFFER_X || tx == ALTAR_OFFER_X + 1) && ty == ALTAR_OFFER_Y) {
        drawOfferAltar(px, py, tx);
        return;
    }

    if (tx >= 5 && tx <= 8 && ty >= 4 && ty <= 6) {
        int lx = tx - 5;
        int ly = ty - 4;
        uint16_t stone = 0x31AA;
        uint16_t rim = 0x9B15;
        uint16_t glow = 0xF81F;
        uint16_t deep = 0x0000;

        if (ly == 0) {
            lcd.fillRect(px, py + 6, 16, 8, stone);
            lcd.drawFastHLine(px + 1, py + 5, 14, rim);
            lcd.drawFastHLine(px + 2, py + 6, 12, 0xD6BA);
            
            if (lx == 0) {
                lcd.fillRect(px + 10, py + 8, 6, 5, deep);
                lcd.drawFastVLine(px + 3, py + 6, 7, 0xC618);
                lcd.drawFastHLine(px + 10, py + 8, 5, 0x600C);
            } else if (lx == 1 || lx == 2) {
                lcd.fillRect(px, py + 8, 16, 5, deep);
                lcd.drawPixel(px + 6, py + 9, glow);
                lcd.drawPixel(px + 10, py + 9, glow);
                lcd.drawFastHLine(px + 4, py + 8, 8, 0x600C);
            } else {
                lcd.fillRect(px, py + 8, 6, 5, deep);
                lcd.drawFastVLine(px + 12, py + 6, 7, 0xC618);
                lcd.drawFastHLine(px + 2, py + 8, 5, 0x600C);
            }
            
            lcd.drawFastHLine(px + 2, py + 13, 12, 0x2108);
        } else if (ly == 1) {
            lcd.fillRect(px + 1, py, 14, 16, stone);
            
            if (lx == 0) {
                lcd.fillRect(px + 9, py, 7, 11, deep);
                lcd.drawFastVLine(px + 2, py + 2, 11, 0x2108);
                lcd.fillRect(px + 5, py + 11, 4, 2, 0xA81F);
            } else if (lx == 1 || lx == 2) {
                lcd.fillRect(px, py, 16, 10, deep);
                lcd.fillRect(px + 3, py + 10, 10, 3, 0x8010);
                lcd.drawPixel(px + 3, py + 5, glow);
                lcd.drawPixel(px + 12, py + 5, glow);
            } else {
                lcd.fillRect(px, py, 7, 11, deep);
                lcd.drawFastVLine(px + 13, py + 2, 11, 0x2108);
                lcd.fillRect(px + 7, py + 11, 4, 2, 0xA81F);
            }
            
            lcd.drawFastHLine(px + 2, py + 14, 12, 0xC618);
        } else {
            lcd.fillRect(px + 1, py + 1, 14, 11, stone);
            
            if (lx == 0) {
                lcd.fillRect(px + 8, py + 2, 6, 9, 0x294A);
                lcd.fillRect(px + 11, py + 5, 3, 5, 0x8410);
            } else if (lx == 1 || lx == 2) {
                lcd.fillRect(px + 2, py + 3, 12, 7, 0x4208);
                lcd.fillRect(px + 5, py + 5, 6, 3, 0xA81F);
                lcd.drawPixel(px + 7, py + 4, glow);
                lcd.drawPixel(px + 8, py + 4, glow);
            } else {
                lcd.fillRect(px + 2, py + 2, 6, 9, 0x294A);
                lcd.fillRect(px + 2, py + 5, 3, 5, 0x8410);
            }
            
            lcd.drawFastHLine(px + 2, py + 12, 12, 0xC618);
            lcd.drawFastHLine(px + 3, py + 13, 10, 0x2108);
        }
        return;
    }

    lcd.fillRect(px + 3, py + 5, 10, 7, 0x4208);
    lcd.drawRect(px + 3, py + 5, 10, 7, 0x8410);
    lcd.drawPixel(px + 6, py + 7, 0xF81F);
    lcd.drawPixel(px + 9, py + 8, 0xA81F);
}

// Check if player is near the priest NPC
bool altarNearPriest(int playerX, int playerY) {
    return abs(playerX - ALTAR_PRIEST_X) <= 1 &&
           abs(playerY - ALTAR_PRIEST_Y) <= 1;
}

// Check if player is near the skull object
bool altarNearSkull(int playerX, int playerY) {
    return abs(playerX - ALTAR_SKULL_X) <= 1 &&
           abs(playerY - ALTAR_SKULL_Y) <= 1;
}

// Check if player is near the offering altar area
bool altarNearOffering(int playerX, int playerY) {
    return ((abs(playerX - ALTAR_OFFER_X) <= 1 && abs(playerY - ALTAR_OFFER_Y) <= 1) ||
            (abs(playerX - (ALTAR_OFFER_X + 1)) <= 1 && abs(playerY - ALTAR_OFFER_Y) <= 1));
}