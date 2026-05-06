#include "ui.h"
#include <math.h>
#include "config.h"
#include "st7789.h"
#include "drop.h"
#include "enemy.h"
#include "weapon.h"
#include "portal.h"
#include "character.h"
#include "worldmap.h"
#include "console.h"
#include "altar.h"
#include "supply.h"
#include "base_sprites.h"
#include "farm_bg.h"
#include "lost_bg.h"
#include "factory_bg.h"
#include "base_bg.h"

// Current level area (controls tile visuals)
static int s_areaId = AREA_INDUSTRIAL;
static int s_cameraX = 0;

// Forward declaration for sprite scaling
static int combatScale();

// -----------------------------------------------------------------------------
// Public UI State Setters
// -----------------------------------------------------------------------------
void uiSetArea(int areaId) {
    s_areaId = areaId;
}

void uiSetCameraX(int cx) {
    s_cameraX = cx;
}

// -----------------------------------------------------------------------------
// Top Status Bar (HP / Hunger / Armor)
// -----------------------------------------------------------------------------
void drawStatusBar(Player* p) {
    lcd.fillRect(0, 0, 240, STATUS_BAR_H, COLOR_BG_PANEL);

    // HP Bar
    lcd.drawText(4, 8, "HP", COLOR_WHITE, COLOR_BG_PANEL, 1);
    lcd.drawRect(22, 7, 50, 8, COLOR_WHITE);
    lcd.fillRect(23, 8, (p->hp * 48) / 100, 6, COLOR_HP);

    // Hunger Bar
    lcd.drawText(82, 8, "HNG", COLOR_WHITE, COLOR_BG_PANEL, 1);
    lcd.drawRect(106, 7, 50, 8, COLOR_WHITE);
    lcd.fillRect(107, 8, (p->hunger * 48) / 100, 6, COLOR_HUNGER);

    // Armor Bar
    lcd.drawText(166, 8, "ARM", COLOR_WHITE, COLOR_BG_PANEL, 1);
    lcd.drawRect(190, 7, 46, 8, COLOR_WHITE);
    lcd.fillRect(191, 8, (p->armor * 44) / 100, 6, COLOR_ARMOR);
}

// -----------------------------------------------------------------------------
// Weapon Icon Rendering (Bottom Bar Slot 1)
// -----------------------------------------------------------------------------
static void drawWeaponIcon(int weaponType) {
    lcd.fillRect(5, 205, 70, 26, COLOR_BG_PANEL);

    switch (weaponType) {
        case WEAPON_SMG:
            lcd.fillRect(8, 210, 24, 6, COLOR_LIGHTGRAY);
            lcd.fillRect(32, 211, 14, 4, COLOR_LIGHTGRAY);
            lcd.fillRect(14, 216, 6, 9, COLOR_GRAY);
            lcd.drawText(20, 221, "SMG", COLOR_CYAN, COLOR_BG_PANEL, 1);
            break;

        case WEAPON_MG:
            lcd.fillRect(6, 209, 26, 8, COLOR_LIGHTGRAY);
            lcd.fillRect(32, 210, 22, 6, COLOR_LIGHTGRAY);
            lcd.fillRect(54, 211, 4, 4, COLOR_GRAY);
            lcd.fillRect(12, 217, 8, 9, COLOR_GRAY);
            lcd.fillRect(48, 215, 2, 9, COLOR_GRAY);
            lcd.fillRect(54, 215, 2, 9, COLOR_GRAY);
            lcd.drawText(22, 221, "MG", COLOR_ORANGE, COLOR_BG_PANEL, 1);
            break;

        case WEAPON_LASER:
            lcd.fillRect(10, 212, 10, 4, COLOR_GRAY);
            lcd.fillRect(20, 209, 4, 10, COLOR_LIGHTGRAY);
            lcd.fillRect(24, 211, 40, 6, COLOR_CYAN);
            lcd.fillRect(26, 212, 36, 4, COLOR_WHITE);
            lcd.drawText(18, 222, "L-SWD", COLOR_CYAN, COLOR_BG_PANEL, 1);
            break;

        default: // Laser Gun
            lcd.fillRect(8, 209, 20, 8, COLOR_LIGHTGRAY);
            lcd.fillRect(28, 211, 26, 4, COLOR_CYAN);
            lcd.fillRect(12, 217, 8, 9, COLOR_GRAY);
            lcd.fillRect(20, 207, 6, 4, COLOR_GRAY);
            lcd.drawText(18, 222, "L-GUN", COLOR_CYAN, COLOR_BG_PANEL, 1);
            break;
    }
}

// Redraw only weapon slot (no full bar clear = no flicker)
void drawWeaponSlot(int weaponType) {
    lcd.fillRect(4, 204, 72, 28, COLOR_BG_PANEL);
    lcd.drawRect(4, 204, 72, 28, COLOR_WHITE);
    drawWeaponIcon(weaponType);
}

// -----------------------------------------------------------------------------
// Bottom Action Bar (Weapon / Backpack / Area Name)
// -----------------------------------------------------------------------------
static void drawAreaName(int x, int y) {
    const char* line1;
    const char* line2;

    switch (s_areaId) {
        case AREA_HOME:       line1 = "ASHROOT"; line2 = "FARM";    break;
        case AREA_TRADE:      line1 = "DAWN";    line2 = "SUPPLY";  break;
        case AREA_INDUSTRIAL: line1 = "RUST";    line2 = "FORGE";   break;
        case AREA_FARMLAND:   line1 = "ASHVEIL"; line2 = "LANDS";   break;
        case AREA_RANDOM:     line1 = "LOST";    line2 = "EXPANSE"; break;
        case AREA_BASE:       line1 = "ECLIPSE"; line2 = "BASE";    break;
        case AREA_ALTAR:      line1 = "RELIC";   line2 = "ALTAR";   break;
        default:              line1 = "UNKNOWN"; line2 = "ZONE";    break;
    }

    lcd.drawText(x, y, line1, COLOR_CYAN, COLOR_BG_PANEL, 1);
    lcd.drawText(x, y + 11, line2, COLOR_LIGHTGRAY, COLOR_BG_PANEL, 1);
}

void drawBottomBar(int weaponType) {
    lcd.fillRect(0, 200, 240, 40, COLOR_BG_PANEL);

    // Weapon Slot
    lcd.drawRect(4, 204, 72, 28, COLOR_WHITE);
    drawWeaponIcon(weaponType);

    // Backpack Slot
    lcd.drawRect(84, 204, 72, 28, COLOR_WHITE);
    lcd.fillRect(108, 208, 20, 18, COLOR_GRAY);
    lcd.drawRect(108, 208, 20, 18, COLOR_LIGHTGRAY);
    lcd.fillRect(109, 205, 2, 5, COLOR_GRAY);
    lcd.fillRect(125, 205, 2, 5, COLOR_GRAY);
    lcd.fillRect(110, 205, 16, 4, COLOR_DARKGRAY);
    lcd.fillRect(115, 216, 6, 2, COLOR_YELLOW);
    lcd.drawText(90, 221, "PACK", COLOR_LIGHTGRAY, COLOR_BG_PANEL, 1);

    // Area Name Slot
    lcd.drawRect(164, 204, 72, 28, COLOR_WHITE);
    drawAreaName(172, 210);
}

// -----------------------------------------------------------------------------
// Dropped Item Icons
// -----------------------------------------------------------------------------
static void drawDropIcon(int tileX, int tileY, int itemType) {
    int px = PLAY_AREA_X + tileX * TILE_SIZE + 5 - s_cameraX;
    int py = PLAY_AREA_Y + tileY * TILE_SIZE + 5;

    if (px <= PLAY_AREA_X - TILE_SIZE || px >= PLAY_AREA_X + PLAY_AREA_W)
        return;

    // Medkit special icon
    if (itemType == INV_MEDKIT) {
        int mx = PLAY_AREA_X + tileX * TILE_SIZE + 4 - s_cameraX;
        int my = PLAY_AREA_Y + tileY * TILE_SIZE + 4;
        lcd.fillRect(mx, my, 8, 8, COLOR_RED);
        lcd.drawRect(mx, my, 8, 8, COLOR_WHITE);
        lcd.fillRect(mx+3, my+1, 2, 6, COLOR_WHITE);
        lcd.fillRect(mx+1, my+3, 6, 2, COLOR_WHITE);
        return;
    }

    uint16_t color;
    switch (itemType) {
        case INV_SEEDS:        color = COLOR_GREEN;      break;
        case INV_FOOD:         color = COLOR_ORANGE;     break;
        case INV_METAL:        color = COLOR_LIGHTGRAY;  break;
        case INV_ALIEN_RELICS: color = COLOR_MAGENTA;    break;
        default:               color = COLOR_WHITE;       break;
    }

    lcd.fillRect(px, py, 6, 6, color);
    lcd.drawRect(px, py, 6, 6, COLOR_WHITE);
}

static void drawAllDrops() {
    for (int i = 0; i < MAX_DROPS; i++) {
        if (drops[i].active) {
            drawDropIcon(drops[i].tileX, drops[i].tileY, drops[i].itemType);
        }
    }
}

// -----------------------------------------------------------------------------
// Tile Rendering (Walls / Ground / Decor / Blockers)
// -----------------------------------------------------------------------------
static void drawWall(int px, int py, int tx, int ty) {
    switch (s_areaId) {
        case AREA_FARMLAND:
            lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x8220);
            lcd.drawFastVLine(px+3, py, TILE_SIZE, 0x4120);
            lcd.drawFastVLine(px+8, py, TILE_SIZE, 0x4120);
            lcd.drawFastVLine(px+12, py, TILE_SIZE, 0x4120);
            lcd.drawFastHLine(px, py+7, TILE_SIZE, 0x3900);
            if (((tx*7 + ty*5) & 3) == 0) lcd.fillRect(px+5, py+2, 3, 3, 0x3900);
            if (((tx*3 + ty*9) & 3) == 1) lcd.fillRect(px+9, py+10, 2, 2, 0x3900);
            lcd.drawFastVLine(px+15, py, TILE_SIZE, 0x2900);
            lcd.drawFastHLine(px, py+15, TILE_SIZE, 0x2900);
            break;

        case AREA_RANDOM:
            lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x6B6D);
            lcd.drawFastHLine(px, py+7, TILE_SIZE, 0x39E7);
            lcd.drawFastVLine(px+7, py, 8, 0x39E7);
            lcd.drawFastVLine(px+7, py+9, 7, 0x4228);
            if (((tx*5 + ty*3) & 3) == 0) {
                lcd.drawFastHLine(px+2, py+3, 5, 0x2104);
                lcd.drawFastVLine(px+6, py+3, 4, 0x2104);
            }
            if (((tx*9 + ty*7) & 7) == 2)
                lcd.drawFastHLine(px+8, py+11, 6, 0x2104);
            if (((tx+ty*11) &7) <3) {
                lcd.drawPixel(px+4, py+12, 0x9CF3);
                lcd.drawPixel(px+11, py+5, 0x9CF3);
            }
            lcd.drawFastVLine(px+15, py, TILE_SIZE, 0x2104);
            lcd.drawFastHLine(px, py+15, TILE_SIZE, 0x2104);
            break;

        case AREA_BASE:
            lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x0208);
            lcd.drawFastHLine(px, py+5, TILE_SIZE, 0x0330);
            lcd.drawFastHLine(px, py+11, TILE_SIZE, 0x0330);
            lcd.drawFastVLine(px+5, py, TILE_SIZE, 0x0330);
            lcd.drawFastVLine(px+11, py, TILE_SIZE, 0x0330);
            lcd.drawPixel(px+5, py+5, 0x07FF);
            lcd.drawPixel(px+11, py+5, 0x07FF);
            lcd.drawPixel(px+5, py+11, 0x07FF);
            lcd.drawPixel(px+11, py+11, 0x07FF);
            lcd.drawFastHLine(px, py, TILE_SIZE, 0x041F);
            lcd.drawFastVLine(px+15, py, TILE_SIZE, 0x0004);
            break;

        default: // Industrial
            lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x4228);
            lcd.drawFastHLine(px, py+5, TILE_SIZE, 0x2104);
            lcd.drawFastHLine(px, py+11, TILE_SIZE, 0x2104);
            lcd.fillRect(px+1, py+4, 2,2, 0x8410);
            lcd.fillRect(px+13, py+4,2,2, 0x8410);
            lcd.fillRect(px+1, py+10,2,2, 0x8410);
            lcd.fillRect(px+13, py+10,2,2, 0x8410);
            if (((tx*5+ty*11)&3)==0) lcd.fillRect(px+2,py+1,4,3,0xA300);
            if (((tx*11+ty*7)&7)==2) lcd.fillRect(px+10,py+6,4,4,0x8240);
            if (((tx*3+ty*13)&7)==5) lcd.fillRect(px+5,py+12,5,3,0x8240);
            lcd.drawFastHLine(px,py,TILE_SIZE,0xA940);
            lcd.drawFastVLine(px+15,py,TILE_SIZE,0x2104);
            lcd.drawFastHLine(px,py+15,TILE_SIZE,0x2104);
            break;
    }
}

static void drawThematic(int px, int py, int tx, int ty) {
    switch (s_areaId) {
        case AREA_FARMLAND:
            lcd.fillRect(px,py,TILE_SIZE,TILE_SIZE,0x4A26);
            lcd.drawFastHLine(px,py+4,TILE_SIZE,0x3165);
            lcd.drawFastHLine(px,py+12,TILE_SIZE,0x3165);
            if (((tx*7+ty*5)&3)==0) {
                lcd.drawFastVLine(px+4,py+1,3,0x8240);
                lcd.drawPixel(px+3,py+1,0x8240);
            }
            if (((tx*3+ty*11)&3)==1)
                lcd.drawFastVLine(px+11,py+6,3,0x8240);
            lcd.drawPixel(px+7,py+8,0x9CF3);
            break;

        case AREA_RANDOM:
            lcd.fillRect(px,py,TILE_SIZE,TILE_SIZE,0x2863);
            lcd.drawFastHLine(px+1,py+4,8,0x2FE0);
            lcd.drawFastHLine(px+3,py+11,6,0x1FC0);
            if (((tx*7+ty*3)&3)==0)
                lcd.fillRect(px+2,py+6,3,3,0x3FE0);
            if (((tx*3+ty*7)&7)==3) {
                lcd.drawPixel(px+12,py+3,0x7FE0);
                lcd.drawPixel(px+13,py+4,0x7FE0);
            }
            lcd.drawFastHLine(px,py,TILE_SIZE,0x1780);
            break;

        case AREA_BASE:
            lcd.fillRect(px,py,TILE_SIZE,TILE_SIZE,0x0008);
            lcd.drawFastHLine(px+1,py+5,6,0x8010);
            lcd.drawFastHLine(px+7,py+10,5,0x8010);
            lcd.drawFastVLine(px+8,py+1,5,0x8010);
            if (((tx*7+ty*3)&3)==0)
                lcd.fillRect(px+2,py+2,2,2,0xF81F);
            if (((tx*3+ty*7)&7)==3) {
                lcd.drawPixel(px+12,py+9,0xF81F);
                lcd.drawPixel(px+11,py+8,0xA00C);
            }
            if (((tx+ty*5)&7)==2)
                lcd.fillRect(px+9,py+3,2,2,0x600C);
            break;

        default: // Industrial
            lcd.fillRect(px,py,TILE_SIZE,TILE_SIZE,0x3820);
            lcd.drawFastHLine(px+1,py+5,7,0x2800);
            lcd.drawFastHLine(px+7,py+11,5,0x2800);
            if ((tx+ty)&1)
                lcd.drawFastVLine(px+9,py+2,6,0x2800);
            if (((tx*7+ty*3)&3)==0)
                lcd.fillRect(px+2,py+2,4,4,0xA940);
            if (((tx*3+ty*7)&7)==3)
                lcd.fillRect(px+10,py+9,2,2,0xFD20);
            if (((tx+ty*3)&7)==5)
                lcd.fillRect(px+12,py+3,2,3,0xB920);
            lcd.drawFastHLine(px,py,TILE_SIZE,0x5A00);
            break;
    }
}

static void drawGroundDecorBase(int px, int py, int tx, int ty) {
    switch (s_areaId) {
        case AREA_INDUSTRIAL: {
            int sx = (tx * TILE_SIZE) % FACTORY_BG_WIDTH;
            int sy = (ty * TILE_SIZE) % FACTORY_BG_HEIGHT;
            lcd.drawSubBitmapTransparent(px, py, TILE_SIZE, TILE_SIZE,
                &factory_bg[sy * FACTORY_BG_WIDTH + sx], FACTORY_BG_WIDTH);
            break;
        }
        case AREA_FARMLAND: {
            int sx = (tx * TILE_SIZE) % FARM_BG_WIDTH;
            int sy = (ty * TILE_SIZE) % FARM_BG_HEIGHT;
            lcd.drawSubBitmapTransparent(px, py, TILE_SIZE, TILE_SIZE,
                &farm_bg[sy * FARM_BG_WIDTH + sx], FARM_BG_WIDTH);
            break;
        }
        case AREA_RANDOM: {
            int sx = (tx * TILE_SIZE) % LOST_BG_WIDTH;
            int sy = (ty * TILE_SIZE) % LOST_BG_HEIGHT;
            lcd.drawSubBitmapTransparent(px, py, TILE_SIZE, TILE_SIZE,
                &lost_bg[sy * LOST_BG_WIDTH + sx], LOST_BG_WIDTH);
            break;
        }
        case AREA_BASE: {
            int sx = (tx * TILE_SIZE) % BASE_BG_WIDTH;
            int sy = (ty * TILE_SIZE) % BASE_BG_HEIGHT;
            lcd.drawSubBitmapTransparent(px, py, TILE_SIZE, TILE_SIZE,
                &base_bg[sy * BASE_BG_WIDTH + sx], BASE_BG_WIDTH);
            break;
        }
        case AREA_HOME:
            lcd.fillRect(px,py,TILE_SIZE,TILE_SIZE,0x4A48);
            lcd.drawFastHLine(px,py+12,TILE_SIZE,0x39A6);
            if (!((tx+ty)&1)) lcd.drawFastVLine(px+6,py+3,6,0x52A8);
            lcd.drawPixel(px+11,py+5,0x6B6D);
            break;
        case AREA_TRADE:
            supplyDrawGround(px,py,tx,ty);
            break;
        case AREA_ALTAR:
            altarDrawGround(px,py,tx,ty);
            break;
        default:
            lcd.fillRect(px,py,TILE_SIZE,TILE_SIZE,0x2104);
            lcd.drawFastHLine(px+1,py+5,7,0x4208);
            lcd.drawFastHLine(px+7,py+11,5,0x4208);
            lcd.drawPixel(px+12,py+3,0x7BEF);
            break;
    }
}

static void drawDecorIcon(int px, int py, int tx, int ty) {
    drawGroundDecorBase(px, py, tx, ty);
}

static void drawBlockerIcon(int px, int py, int tx, int ty) {
    drawGroundDecorBase(px, py, tx, ty);
}

static void drawTile(int tx, int ty, uint8_t tile) {
    int px = PLAY_AREA_X + tx * TILE_SIZE - s_cameraX;
    int py = PLAY_AREA_Y + ty * TILE_SIZE;

    if (px <= PLAY_AREA_X - TILE_SIZE || px >= PLAY_AREA_X + PLAY_AREA_W)
        return;

    if (tile == 1)
        drawBlockerIcon(px, py, tx, ty);
    else if (tile == 2)
        drawDecorIcon(px, py, tx, ty);
    else
        drawGroundDecorBase(px, py, tx, ty);
}

// -----------------------------------------------------------------------------
// Player & Enemy Sprite Rendering
// -----------------------------------------------------------------------------
static void drawPlayerAndChargeBar(Player* p, int px, int py, int scale) {
    characterDraw(px, py, p->weapon.type, scale, p->weapon.lastFire);

    if (p->weapon.type == WEAPON_LASER_GUN && p->weapon.isCharging) {
        uint32_t chargeMs = millis() - p->weapon.chargeStartMs;
        if (chargeMs > 1000) chargeMs = 1000;

        int barW = 16 * scale;
        int barH = 4;
        int barX = px + (16 * scale - barW) / 2;
        int barY = py - barH - 4;
        int fillW = ((barW - 2) * chargeMs) / 1000;

        lcd.fillRect(barX, barY, barW, barH, COLOR_BLACK);
        lcd.drawRect(barX, barY, barW, barH, COLOR_WHITE);
        if (fillW > 0) {
            uint16_t color = (chargeMs >= 1000) ? COLOR_RED : COLOR_CYAN;
            lcd.fillRect(barX + 1, barY + 1, fillW, barH - 2, color);
        }
    }
}

static void drawPlayer(Player* p) {
    int px = PLAY_AREA_X + p->pixelX - s_cameraX;
    int py = PLAY_AREA_Y + p->pixelY;
    drawPlayerAndChargeBar(p, px, py, combatScale());
}

// Alien sprites (16x16 bitmaps)
static const uint16_t sAlienBody[16] = {
    0x0780,0x0FC0,0x1FE0,0x1FE0,0x1FE0,0x0FC0,0x0780,0x1FE0,
    0x3FF0,0x3FF0,0x3FF0,0x1FE0,0x1CE0,0x1CE0,0x0C60,0x0C60
};
static const uint16_t sAlienEyes[16] = {
    0,0,0,0x0CC0,0x0CC0,0,0,0,0,0,0,0,0,0,0,0
};
static const uint16_t sKnifeBlade[16] = {
    0,0,0,0,0x0004,0x000C,0x000C,0x0018,0x0018,0x0010,0x0010,0,0,0,0,0
};
static const uint16_t sLaserGun[16] = {
    0,0,0,0,0,0,0,0x003E,0x003F,0x001C,0x0010,0,0,0,0,0
};
static const uint16_t sHeavyBody[16] = {
    0x1818,0x3FFC,0x7FFE,0x7FFE,0x7FFE,0x3FFC,0x7FFE,0xFFFF,
    0xFFFF,0x7FFE,0x7FFE,0x3FFC,0x381C,0x381C,0x381C,0x7C3E
};
static const uint16_t sHeavyEyes[16] = {
    0,0,0,0x1818,0x0810,0,0,0,0,0,0,0,0,0,0,0
};
static const uint16_t sHammer[16] = {
    0,0,0x000E,0x000E,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x000E,0x000E,0,0,0,0
};
static const uint8_t sBeastBody[8]  = {0x00,0x3C,0x7E,0xFF,0xBD,0xFF,0x66,0x42};
static const uint8_t sBeastEyes[8]  = {0x00,0x00,0x24,0x00,0x00,0x00,0x00,0x00};
static const uint8_t sBeastClaws[8] = {0x00,0x00,0x00,0x81,0x42,0x81,0x00,0x00};

// Colors
#define COL_ALIEN_SKIN  0x27E2
#define COL_ALIEN_EYES  0xF800
#define COL_KNIFE       0xC618
#define COL_LASER_GUN   0x07FF
#define COL_HEAVY_SKIN  0x600F
#define COL_HEAVY_EYES  0xFD20
#define COL_HAMMER      0x8410
#define COL_BEAST_SKIN  0x7BE0
#define COL_BEAST_EYES  0xF81F
#define COL_BEAST_CLAW  0xFFFF

// Combat zones use 3x scale; safe zones 2x
static int combatScale() {
    switch (s_areaId) {
        case AREA_INDUSTRIAL:
        case AREA_FARMLAND:
        case AREA_RANDOM:
        case AREA_BASE:
            return 3;
        default:
            return 2;
    }
}

static inline int spriteBitOffset(int col, int outSize) {
    return (col * outSize) >> 4;
}

static void drawSpriteLayer(int px, int py, const uint16_t* data, uint16_t color, int scale) {
    int outSize = 8 * scale;
    int offset = (outSize - 16) / 2;
    int baseX = px - offset;
    int baseY = py - offset;

    for (int row = 0; row < 16; row++) {
        uint16_t bits = data[row];
        if (!bits) continue;

        int y = baseY + spriteBitOffset(row, outSize);
        int h = spriteBitOffset(row+1, outSize) - spriteBitOffset(row, outSize);

        for (int col = 0; col < 16; col++) {
            if ((bits >> (15 - col)) & 1) {
                int x = baseX + spriteBitOffset(col, outSize);
                int w = spriteBitOffset(col+1, outSize) - spriteBitOffset(col, outSize);
                lcd.fillRect(x, y, w, h, color);
            }
        }
    }
}

static void drawEnemySprite(int px, int py, int type, int scale) {
    if (type == ENEMY_KNIFE) {
        drawSpriteLayer(px, py, sAlienBody, COL_ALIEN_SKIN, scale);
        drawSpriteLayer(px, py, sAlienEyes, COL_ALIEN_EYES, scale);
        drawSpriteLayer(px, py, sKnifeBlade, COL_KNIFE, scale);
    } else if (type == ENEMY_LASER) {
        drawSpriteLayer(px, py, sAlienBody, COL_ALIEN_SKIN, scale);
        drawSpriteLayer(px, py, sAlienEyes, COL_ALIEN_EYES, scale);
        drawSpriteLayer(px, py, sLaserGun, COL_LASER_GUN, scale);
    } else {
        drawSpriteLayer(px, py, sHeavyBody, COL_HEAVY_SKIN, scale);
        drawSpriteLayer(px, py, sHeavyEyes, COL_HEAVY_EYES, scale);
        drawSpriteLayer(px, py, sHammer, COL_HAMMER, scale);
    }
}

static void drawEnemies() {
    int scale = combatScale();
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        int px = PLAY_AREA_X + enemies[i].pixelX - s_cameraX;
        int py = PLAY_AREA_Y + enemies[i].pixelY;
        drawEnemySprite(px, py, enemies[i].type, scale);
    }
}

// -----------------------------------------------------------------------------
// Smooth Entity & Projectile Updates (No Flicker)
// -----------------------------------------------------------------------------
static void redrawTileRect(uint8_t mapData[MAP_ROWS][MAP_COLS], int rx, int ry, int rw, int rh) {
    int left = rx;
    int top = ry;
    int right = rx + rw - 1;
    int bottom = ry + rh - 1;

    int pLeft = PLAY_AREA_X;
    int pTop = PLAY_AREA_Y;
    int pRight = PLAY_AREA_X + PLAY_AREA_W - 1;
    int pBottom = PLAY_AREA_Y + PLAY_AREA_H - 1;

    if (left < pLeft) left = pLeft;
    if (top < pTop) top = pTop;
    if (right > pRight) right = pRight;
    if (bottom > pBottom) bottom = pBottom;
    if (left > right || top > bottom) return;

    int tx0 = (left - PLAY_AREA_X + s_cameraX) / TILE_SIZE;
    int ty0 = (top - PLAY_AREA_Y) / TILE_SIZE;
    int tx1 = (right - PLAY_AREA_X + s_cameraX) / TILE_SIZE;
    int ty1 = (bottom - PLAY_AREA_Y) / TILE_SIZE;

    if (tx0 < 0) tx0 = 0;
    if (tx1 >= MAP_COLS) tx1 = MAP_COLS - 1;

    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            drawTile(tx, ty, mapData[ty][tx]);
            if (dropExistsAt(tx, ty))
                drawDropIcon(tx, ty, dropTypeAt(tx, ty));
            if (portal.active && portal.tileX == tx && portal.tileY == ty)
                drawPortalTile(tx, ty);
        }
    }
}

void drawProjectilesUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active && !projectiles[i].pendingErase)
            continue;

        int oldX = projectiles[i].lastDrawnX - s_cameraX;
        int oldY = PLAY_AREA_Y + projectiles[i].lastDrawnY;
        int drawX, drawY, drawW, drawH;

        if (projectiles[i].type == WEAPON_LASER) {
            drawX = PLAY_AREA_X + oldX;
            drawY = oldY;
            drawW = projectiles[i].lastDrawnW;
            drawH = projectiles[i].lastDrawnH;
            drawX -=4; drawY -=4; drawW +=8; drawH +=8;
        } else {
            int sz = 16;
            int rad = sz/2;
            drawX = PLAY_AREA_X + oldX - rad;
            drawY = oldY - rad;
            drawW = sz;
            drawH = sz;
        }

        if (drawX <0) { drawW += drawX; drawX=0; }
        if (drawY <0) { drawH += drawY; drawY=0; }
        if (drawX+drawW >240) drawW=240-drawX;
        if (drawY+drawH >240) drawH=240-drawY;

        if (drawW>0 && drawH>0) {
            redrawTileRect(mapData, drawX, drawY, drawW, drawH);
        }

        if (projectiles[i].active) {
            int newX = projectiles[i].pixelX - s_cameraX;
            int newY = PLAY_AREA_Y + projectiles[i].pixelY;

            if (projectiles[i].type == WEAPON_LASER) {
                // Laser sword swing effect
                int cx = player->pixelX +8;
                int cy = player->pixelY +8;
                float angle = 0.0f;
                float r = (float)projectiles[i].range / 32.0f;

                if (projectiles[i].vx>0) angle = -1.5f + 3.0f*r;
                else if (projectiles[i].vx<0) angle = 3.14159f +1.5f -3.0f*r;
                else if (projectiles[i].vy>0) angle = 1.5708f -1.5f +3.0f*r;
                else if (projectiles[i].vy<0) angle = -1.5708f -1.5f +3.0f*r;

                int len = 10 * combatScale();
                int minX=9999, maxX=-9999, minY=9999, maxY=-9999;

                for (int d=0; d<len; d+=2) {
                    int bx = cx + (int)(cos(angle)*d);
                    int by = cy + (int)(sin(angle)*d);
                    int sx = PLAY_AREA_X + bx - s_cameraX;
                    int sy = PLAY_AREA_Y + by;

                    if (sx>=0 && sx<240 && sy>=PLAY_AREA_Y && sy<240) {
                        lcd.fillRect(sx-2,sy-2,4,4,COLOR_CYAN);
                        lcd.fillRect(sx-1,sy-1,2,2,COLOR_WHITE);
                    }
                    if (bx<minX) minX=bx; if (bx>maxX) maxX=bx;
                    if (by<minY) minY=by; if (by>maxY) maxY=by;
                }

                projectiles[i].lastDrawnX = minX;
                projectiles[i].lastDrawnY = minY;
                projectiles[i].lastDrawnW = maxX-minX;
                projectiles[i].lastDrawnH = maxY-minY;
            } else {
                if (newX>=0 && newX<PLAY_AREA_W && newY>=PLAY_AREA_Y && newY<240) {
                    uint16_t c = COLOR_YELLOW;
                    if (projectiles[i].type == WEAPON_MG) c=COLOR_ORANGE;
                    else if (projectiles[i].type == WEAPON_LASER_GUN) c=COLOR_CYAN;
                    lcd.fillRect(PLAY_AREA_X+newX-1, newY-1,4,4,c);
                }
                projectiles[i].lastDrawnX = projectiles[i].pixelX;
                projectiles[i].lastDrawnY = projectiles[i].pixelY;
            }
        } else if (projectiles[i].pendingErase) {
            projectiles[i].pendingErase = false;
        }
    }
}

// Full smooth movement rendering for player & enemies
void drawSmoothEntitiesUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player) {
    int pCurX = PLAY_AREA_X + player->pixelX - s_cameraX;
    int pCurY = PLAY_AREA_Y + player->pixelY;

    uint32_t swing = millis() - player->weapon.lastFire;
    bool swinging = (player->weapon.type == WEAPON_LASER && swing < 300);
    bool justSwing = (player->weapon.type == WEAPON_LASER && swing >=300 && swing <450);
    bool charging = (player->weapon.type == WEAPON_LASER_GUN && player->weapon.isCharging);
    static bool wasCharging = false;
    bool chargeChanged = (charging != wasCharging);
    wasCharging = charging;

    bool pMoved = (pCurX != player->lastDrawnPixelX || pCurY != player->lastDrawnPixelY ||
                   swinging || justSwing || charging || chargeChanged);

    int eCurX[MAX_ENEMIES];
    int eCurY[MAX_ENEMIES];
    bool eMoved[MAX_ENEMIES] = {false};
    bool anyMoved = pMoved;

    for (int i=0; i<MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        eCurX[i] = PLAY_AREA_X + enemies[i].pixelX - s_cameraX;
        eCurY[i] = PLAY_AREA_Y + enemies[i].pixelY;
        eMoved[i] = (eCurX[i] != enemies[i].lastDrawnPixelX || eCurY[i] != enemies[i].lastDrawnPixelY);
        if (eMoved[i]) anyMoved = true;
    }

    if (!anyMoved) return;

    int scale = combatScale();
    int size = 8 * scale;
    int off = (size -16)/2;
    static uint16_t dirtyBuf[4096];

    // Redraw enemies
    for (int i=0; i<MAX_ENEMIES; i++) {
        if (!enemies[i].active || !eMoved[i]) continue;
        int oldX = enemies[i].lastDrawnPixelX - off;
        int oldY = enemies[i].lastDrawnPixelY - off;
        int newX = eCurX[i] - off;
        int newY = eCurY[i] - off;

        int minX = (oldX < newX) ? oldX : newX;
        int minY = (oldY < newY) ? oldY : newY;
        int maxX = (oldX > newX) ? oldX : newX;
        int maxY = (oldY > newY) ? oldY : newY;
        int rw = (maxX + size) - minX;
        int rh = (maxY + size) - minY;

        if (minX < PLAY_AREA_X) { rw -= PLAY_AREA_X - minX; minX = PLAY_AREA_X; }
        if (minY < PLAY_AREA_Y) { rh -= PLAY_AREA_Y - minY; minY = PLAY_AREA_Y; }
        if (minX+rw > PLAY_AREA_X+PLAY_AREA_W) rw = PLAY_AREA_X+PLAY_AREA_W - minX;
        if (minY+rh > PLAY_AREA_Y+PLAY_AREA_H) rh = PLAY_AREA_Y+PLAY_AREA_H - minY;
        if (rw<=0 || rh<=0) continue;

        if (rw*rh <= 4096) {
            lcd.setRenderTarget(dirtyBuf, minX, minY, rw, rh);
            redrawTileRect(mapData, minX, minY, rw, rh);
            for (int e=0; e<MAX_ENEMIES; e++) if (enemies[e].active)
                drawEnemySprite(eCurX[e], eCurY[e], enemies[e].type, scale);
            drawPlayerAndChargeBar(player, pCurX, pCurY, scale);
            lcd.setRenderTarget(NULL,0,0,0,0);
            lcd.pushImage(minX, minY, rw, rh, dirtyBuf);
        } else {
            redrawTileRect(mapData, minX, minY, rw, rh);
            for (int e=0; e<MAX_ENEMIES; e++) if (enemies[e].active)
                drawEnemySprite(eCurX[e], eCurY[e], enemies[e].type, scale);
            drawPlayerAndChargeBar(player, pCurX, pCurY, scale);
        }
    }

    // Redraw player
    if (pMoved) {
        int oldX = player->lastDrawnPixelX - off;
        int oldY = player->lastDrawnPixelY - off;
        int newX = pCurX - off;
        int newY = pCurY - off;

        int minX = (oldX < newX) ? oldX : newX;
        int minY = (oldY < newY) ? oldY : newY;
        int maxX = (oldX > newX) ? oldX : newX;
        int maxY = (oldY > newY) ? oldY : newY;

        if (player->weapon.type == WEAPON_LASER) {
            int pad = 12 * scale;
            minX -= pad;
            maxX += pad;
        }
        if (player->weapon.type == WEAPON_LASER_GUN) {
            int pad = 16 * scale;
            minY -= pad;
        }

        int rw = (maxX + size) - minX;
        int rh = (maxY + size) - minY;

        if (minX < PLAY_AREA_X) { rw -= PLAY_AREA_X - minX; minX = PLAY_AREA_X; }
        if (minY < PLAY_AREA_Y) { rh -= PLAY_AREA_Y - minY; minY = PLAY_AREA_Y; }
        if (minX+rw > PLAY_AREA_X+PLAY_AREA_W) rw = PLAY_AREA_X+PLAY_AREA_W - minX;
        if (minY+rh > PLAY_AREA_Y+PLAY_AREA_H) rh = PLAY_AREA_Y+PLAY_AREA_H - minY;

        if (rw>0 && rh>0) {
            if (rw*rh <=4096) {
                lcd.setRenderTarget(dirtyBuf, minX, minY, rw, rh);
                redrawTileRect(mapData, minX, minY, rw, rh);
                for (int e=0; e<MAX_ENEMIES; e++) if (enemies[e].active)
                    drawEnemySprite(eCurX[e], eCurY[e], enemies[e].type, scale);
                drawPlayerAndChargeBar(player, pCurX, pCurY, scale);
                lcd.setRenderTarget(NULL,0,0,0,0);
                lcd.pushImage(minX, minY, rw, rh, dirtyBuf);
            } else {
                redrawTileRect(mapData, minX, minY, rw, rh);
                for (int e=0; e<MAX_ENEMIES; e++) if (enemies[e].active)
                    drawEnemySprite(eCurX[e], eCurY[e], enemies[e].type, scale);
                drawPlayerAndChargeBar(player, pCurX, pCurY, scale);
            }
        }
    }

    // Save positions
    player->lastDrawnPixelX = pCurX;
    player->lastDrawnPixelY = pCurY;
    for (int i=0; i<MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        enemies[i].lastDrawnPixelX = eCurX[i];
        enemies[i].lastDrawnPixelY = eCurY[i];
    }
}

// -----------------------------------------------------------------------------
// VFX: Bullet / Laser / Cannon Flash
// -----------------------------------------------------------------------------
static void restoreInterpolatedEntities(Player* player) {
    int scale = combatScale();
    for (int i=0; i<MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        int x = PLAY_AREA_X + enemies[i].pixelX - s_cameraX;
        int y = PLAY_AREA_Y + enemies[i].pixelY;
        drawEnemySprite(x,y,enemies[i].type,scale);
        enemies[i].lastDrawnPixelX = x;
        enemies[i].lastDrawnPixelY = y;
    }
    if (player) {
        int x = PLAY_AREA_X + player->pixelX - s_cameraX;
        int y = PLAY_AREA_Y + player->pixelY;
        drawPlayerAndChargeBar(player,x,y,combatScale());
        player->lastDrawnPixelX = x;
        player->lastDrawnPixelY = y;
    }
}

void drawBulletFlash(uint8_t mapData[MAP_ROWS][MAP_COLS], int px, int py, int tx, int ty, int wtype, Player* player) {
    uint16_t c = COLOR_YELLOW;
    if (wtype == WEAPON_MG) c=COLOR_ORANGE;
    else if (wtype == WEAPON_LASER) c=COLOR_CYAN;

    int pathX[16], pathY[16], len=0;
    int x0=px,y0=py,x1=tx,y1=ty;
    int dx=abs(x1-x0), sx=x0<x1?1:-1;
    int dy=-abs(y1-y0), sy=y0<y1?1:-1;
    int err=dx+dy;

    while (len<16) {
        pathX[len]=x0; pathY[len]=y0; len++;
        if (x0==x1&&y0==y1) break;
        int e2=2*err;
        if (e2>=dy) {err+=dy; x0+=sx;}
        if (e2<=dx) {err+=dx; y0+=sy;}
    }

    for (int i=1; i<len; i++) {
        int cx = PLAY_AREA_X + pathX[i]*TILE_SIZE +7 -s_cameraX;
        int cy = PLAY_AREA_Y + pathY[i]*TILE_SIZE +7;
        if (cx < PLAY_AREA_X || cx >= PLAY_AREA_X+PLAY_AREA_W) continue;
        int sz = (i==len-1)?5:3;
        lcd.fillRect(cx-sz/2, cy-sz/2, sz, sz, c);
    }

    delay(70);

    for (int i=1; i<len; i++)
        drawTile(pathX[i], pathY[i], mapData[pathY[i]][pathX[i]]);

    restoreInterpolatedEntities(player);
}

void drawLaserFlash(uint8_t mapData[MAP_ROWS][MAP_COLS], int sx, int sy, int ex, int ey, Player* player) {
    int pathX[16], pathY[16], len=0;
    int cx=sx, cy=sy;
    int dx = ex>sx?1:(ex<sx?-1:0);
    int dy = ey>sy?1:(ey<sy?-1:0);

    while (len<16) {
        pathX[len]=cx; pathY[len]=cy; len++;
        if (cx==ex&&cy==ey) break;
        cx+=dx; cy+=dy;
    }

    for (int i=1; i<len; i++) {
        int x = PLAY_AREA_X + pathX[i]*TILE_SIZE +7 -s_cameraX;
        int y = PLAY_AREA_Y + pathY[i]*TILE_SIZE +7;
        if (x<PLAY_AREA_X || x>=PLAY_AREA_X+PLAY_AREA_W) continue;
        int sz = (i==len-1)?5:3;
        lcd.fillRect(x-sz/2,y-sz/2,sz,sz,0xF800);
        lcd.fillRect(x,y,1,1,0x07FF);
    }

    delay(60);

    for (int i=1; i<len; i++)
        drawTile(pathX[i],pathY[i],mapData[pathY[i]][pathX[i]]);

    restoreInterpolatedEntities(player);
}

extern int g_recoilTiltAmount;
extern int g_recoilDir;

void drawLaserCannonFlash(uint8_t mapData[MAP_ROWS][MAP_COLS], int sx, int sy, int dir, bool plat, Player* player, int rx, int ry) {
    int cam = s_cameraX;
    float px = player->pixelX;
    float py = player->pixelY;
    int steps =8;
    float stepX = rx/(float)steps;
    float stepY = ry/(float)steps;
    g_recoilDir = dir;
    int scale = combatScale();
    int beamX=0, beamY=0, beamW=0, beamH=0;

    for (int f=0; f<steps; f++) {
        int oldX = (int)px;
        int oldY = (int)py;
        px += stepX;
        py += stepY;

        if (px<0) px=0;
        if (px>MAP_COLS*TILE_SIZE-16) px=MAP_COLS*TILE_SIZE-16;
        if (py<0) py=0;
        if (py>MAP_ROWS*TILE_SIZE-16) py=MAP_ROWS*TILE_SIZE-16;

        player->pixelX = (int)px;
        player->pixelY = (int)py;
        player->tileX = (player->pixelX+8)/TILE_SIZE;
        player->tileY = (player->pixelY+8)/TILE_SIZE;

        int curX = PLAY_AREA_X + player->pixelX - cam;
        int curY = PLAY_AREA_Y + player->pixelY;
        int bx=curX, by=curY, bw=16, bh=16;

        if (dir==CHAR_DIR_LEFT)  { bx=PLAY_AREA_X; bw=curX-PLAY_AREA_X+8; by=curY-8; bh=32; }
        if (dir==CHAR_DIR_RIGHT) { bx=curX+8; bw=PLAY_AREA_X+PLAY_AREA_W-bx; by=curY-8; bh=32; }
        if (dir==CHAR_DIR_UP)    { bx=curX-8; bw=32; by=PLAY_AREA_Y; bh=curY-PLAY_AREA_Y+8; }
        if (dir==CHAR_DIR_DOWN)  { bx=curX-8; bw=32; by=curY+8; bh=PLAY_AREA_Y+PLAY_AREA_H-by; }

        if (bx<PLAY_AREA_X) { bw-=PLAY_AREA_X-bx; bx=PLAY_AREA_X; }
        if (by<PLAY_AREA_Y) { bh-=PLAY_AREA_Y-by; by=PLAY_AREA_Y; }
        if (bx+bw>PLAY_AREA_X+PLAY_AREA_W) bw=PLAY_AREA_X+PLAY_AREA_W-bx;
        if (by+bh>PLAY_AREA_Y+PLAY_AREA_H) bh=PLAY_AREA_Y+PLAY_AREA_H-by;

        redrawTileRect(mapData, PLAY_AREA_X+oldX-cam-16, PLAY_AREA_Y+oldY-16,48,48);

        if (bw>0&&bh>0) {
            if (f%2==0) {
                lcd.fillRect(bx,by,bw,bh,COLOR_CYAN);
                if (dir==CHAR_DIR_LEFT||dir==CHAR_DIR_RIGHT)
                    lcd.fillRect(bx,by+8,bw,16,COLOR_WHITE);
                else
                    lcd.fillRect(bx+8,by,16,bh,COLOR_WHITE);
            } else {
                if (dir==CHAR_DIR_LEFT||dir==CHAR_DIR_RIGHT) {
                    lcd.fillRect(bx,by+4,bw,24,0x03EF);
                    lcd.fillRect(bx,by+12,bw,8,COLOR_CYAN);
                } else {
                    lcd.fillRect(bx+4,by,24,bh,0x03EF);
                    lcd.fillRect(bx+12,by,8,bh,COLOR_CYAN);
                }
            }
            beamX=bx; beamY=by; beamW=bw; beamH=bh;
        }

        g_recoilTiltAmount = (f<3)?(f*2):(steps-f);
        drawPlayerAndChargeBar(player, curX, curY, scale);
        delay(25);
    }

    g_recoilTiltAmount =0;
    if (beamW>0&&beamH>0)
        redrawTileRect(mapData, beamX, beamY, beamW, beamH);

    int fx = PLAY_AREA_X + player->pixelX - cam;
    int fy = PLAY_AREA_Y + player->pixelY;
    redrawTileRect(mapData, fx-16,fy-16,48,48);
    drawAllDrops();
    drawPortal();
    restoreInterpolatedEntities(player);
}

// -----------------------------------------------------------------------------
// Menus & Screens
// -----------------------------------------------------------------------------
void drawBootScreen() {
    lcd.fillScreen(COLOR_BLACK);
}

void drawMainMenu() {
    consoleDrawMainMenu();
}

void drawExploreScreen(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player) {
    lcd.fillScreen(COLOR_BLACK);
    drawStatusBar(player);
    drawBottomBar(player->weapon.type);

    int start = s_cameraX / TILE_SIZE;
    int end = start + VISIBLE_COLS +1;
    if (end>MAP_COLS) end=MAP_COLS;
    static uint16_t rowBuf[PLAY_AREA_W * TILE_SIZE];

    for (int y=0; y<MAP_ROWS; y++) {
        int py = PLAY_AREA_Y + y*TILE_SIZE;
        if (py+TILE_SIZE <= PLAY_AREA_Y || py >= PLAY_AREA_Y+PLAY_AREA_H) continue;
        int rh = TILE_SIZE;
        if (py<PLAY_AREA_Y) { rh -= PLAY_AREA_Y-py; py=PLAY_AREA_Y; }
        if (py+rh > PLAY_AREA_Y+PLAY_AREA_H) rh = PLAY_AREA_Y+PLAY_AREA_H - py;

        lcd.setRenderTarget(rowBuf, PLAY_AREA_X, py, PLAY_AREA_W, rh);
        for (int x=start; x<end; x++)
            drawTile(x,y,mapData[y][x]);
        lcd.setRenderTarget(NULL,0,0,0,0);
        lcd.pushImage(PLAY_AREA_X, py, PLAY_AREA_W, rh, rowBuf);
    }

    drawAllDrops();
    drawPortal();
    drawEnemies();
    drawPlayer(player);
}

void drawExploreUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player, int oldX, int oldY) {
    drawPlayer(player);
    drawTile(oldX, oldY, mapData[oldY][oldX]);
    if (portal.active && portal.tileX == oldX && portal.tileY == oldY)
        drawPortalTile(oldX, oldY);
    if (dropExistsAt(oldX, oldY))
        drawDropIcon(oldX, oldY, dropTypeAt(oldX, oldY));
}

void drawPlayArea(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player) {
    drawExploreScreen(mapData, player);
}

// -----------------------------------------------------------------------------
// Inventory Overlay
// -----------------------------------------------------------------------------
#define INV_X 14
#define INV_Y 28
#define INV_W 212
#define INV_H 170

static void drawItemRow(int y, uint16_t dot, const char* label, int qty) {
    lcd.fillRect(INV_X+8, y+3,8,8,dot);
    lcd.drawRect(INV_X+8, y+3,8,8,COLOR_WHITE);
    lcd.drawText(INV_X+22, y, label, COLOR_WHITE, COLOR_BG_PANEL,1);
    char buf[8];
    sprintf(buf,"x%d",qty);
    lcd.drawText(INV_X+INV_W-30, y, buf, COLOR_YELLOW, COLOR_BG_PANEL,1);
}

void drawInventoryFoodCount(int qty) {
    int x = INV_X + INV_W -30;
    int y = INV_Y +52;
    lcd.fillRect(x,y,30,9,COLOR_BG_PANEL);
    char buf[8];
    sprintf(buf,"x%d",qty);
    lcd.drawText(x,y,buf,COLOR_YELLOW,COLOR_BG_PANEL,1);
}

void drawInventoryArmorCount(int qty) {
    int x = INV_X + INV_W -30;
    int y = INV_Y +104;
    lcd.fillRect(x,y,30,9,COLOR_BG_PANEL);
    char buf[8];
    sprintf(buf,"x%d",qty);
    lcd.drawText(x,y,buf,COLOR_YELLOW,COLOR_BG_PANEL,1);
}

void drawInventoryOverlay(Player* player) {
    lcd.fillRect(INV_X,INV_Y,INV_W,INV_H,COLOR_BG_PANEL);
    lcd.drawRect(INV_X,INV_Y,INV_W,INV_H,COLOR_LIGHTGRAY);
    lcd.fillRect(INV_X+1,INV_Y+1,INV_W-2,18,COLOR_DARKGRAY);
    lcd.drawText(INV_X+66, INV_Y+5, "BACKPACK", COLOR_YELLOW, COLOR_DARKGRAY,1);

    int top = INV_Y+26;
    drawItemRow(top+0, COLOR_GREEN, "Seeds", player->inv.seeds);
    drawItemRow(top+26, COLOR_ORANGE, "Food", player->inv.food);
    drawItemRow(top+52, COLOR_LIGHTGRAY, "Metal", player->inv.metal);
    drawItemRow(top+78, COLOR_CYAN, "Armor", player->inv.armor);
    drawItemRow(top+104, COLOR_MAGENTA, "Alien Relics", player->inv.alienRelics);

    lcd.drawText(INV_X+8, INV_Y+INV_H-11, "B:FOOD", COLOR_ORANGE, COLOR_BG_PANEL,1);
    lcd.drawText(INV_X+64, INV_Y+INV_H-11, "C:ARMOR", COLOR_CYAN, COLOR_BG_PANEL,1);
    lcd.drawText(INV_X+142, INV_Y+INV_H-11, "JOY:CLOSE", COLOR_GRAY, COLOR_BG_PANEL,1);
}

// -----------------------------------------------------------------------------
// Portal
// -----------------------------------------------------------------------------
void drawPortalTile(int tx, int ty) {
    int px = PLAY_AREA_X + tx*TILE_SIZE - s_cameraX;
    int py = PLAY_AREA_Y + ty*TILE_SIZE;
    if (px <= PLAY_AREA_X-TILE_SIZE || px >= PLAY_AREA_X+PLAY_AREA_W) return;

    lcd.fillRect(px,py,16,16,0x0803);
    lcd.fillRect(px+2,py+2,12,12,0x2008);
    lcd.fillRect(px+4,py+4,8,8,0x400C);
    lcd.fillRect(px+6,py+6,4,4,0x1005);
    lcd.fillRect(px+7,py+7,2,2,0x0000);

    lcd.fillRect(px+7,py+4,2,1,COLOR_ORANGE);
    lcd.fillRect(px+7,py+11,2,1,COLOR_ORANGE);
    lcd.fillRect(px+4,py+7,1,2,COLOR_ORANGE);
    lcd.fillRect(px+11,py+7,1,2,COLOR_ORANGE);

    lcd.drawPixel(px+5,py+5,COLOR_CYAN);
    lcd.drawPixel(px+9,py+5,COLOR_CYAN);
    lcd.drawPixel(px+5,py+9,COLOR_CYAN);
    lcd.drawPixel(px+9,py+9,COLOR_CYAN);
}

void drawPortal() {
    if (portal.active)
        drawPortalTile(portal.tileX, portal.tileY);
}

void drawPortalSlot(bool near) {
    lcd.fillRect(165,205,70,26,COLOR_BG_PANEL);
    if (near) {
        lcd.drawText(191,210,"[A]",COLOR_CYAN,COLOR_BG_PANEL,1);
        lcd.drawText(185,221,"ENTER",COLOR_YELLOW,COLOR_BG_PANEL,1);
    } else {
        drawAreaName(172,210);
    }
}

