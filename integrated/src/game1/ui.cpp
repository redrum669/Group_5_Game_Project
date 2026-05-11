#include "game1/systems/ui.h"
#include <math.h>
#include "config.h"
#include "st7789.h"
#include "game1/entities/drop.h"
#include "game1/entities/enemy.h"
#include "game1/entities/weapon.h"
#include "game1/entities/portal.h"
#include "game1/entities/character.h"
#include "game1/scenes/worldmap.h"
#include "game1/scenes/console.h"
#include "game1/entities/altar.h"
#include "game1/entities/supply.h"
#include "base_sprites.h"
#include "farm_bg.h"
#include "lost_bg.h"
#include "factory_bg.h"
#include "base_bg.h"

// Current area �?set once when entering a level; controls all tile visuals
static int s_areaId = AREA_INDUSTRIAL;
static int s_cameraX = 0;

void uiSetArea(int areaId) {
    s_areaId = areaId;
}

void uiSetCameraX(int cx) {
    s_cameraX = cx;
}

// Forward declared so play-area draw helpers can query the current sprite
// scale without reordering the sprite-helper definitions below.
static int combatScale();


void drawStatusBar(Player* p) {
    lcd.fillRect(0, 0, 240, STATUS_BAR_H, COLOR_BG_PANEL);

    // HP  (x=4)
    lcd.drawText(4,  8, "HP",  COLOR_WHITE, COLOR_BG_PANEL, 1);
    lcd.drawRect(22, 7, 50, 8, COLOR_WHITE);
    lcd.fillRect(23, 8, (p->hp * 48) / 100, 6, COLOR_HP);

    // Hunger  (x=82)
    lcd.drawText(82,  8, "HNG", COLOR_WHITE, COLOR_BG_PANEL, 1);
    lcd.drawRect(106, 7, 50, 8, COLOR_WHITE);
    lcd.fillRect(107, 8, (p->hunger * 48) / 100, 6, COLOR_HUNGER);

    // Armor  (x=166)
    lcd.drawText(166, 8, "ARM", COLOR_WHITE, COLOR_BG_PANEL, 1);
    lcd.drawRect(190, 7, 46, 8, COLOR_WHITE);
    lcd.fillRect(191, 8, (p->armor * 44) / 100, 6, COLOR_ARMOR);
}

// Draw pixel-art weapon icon inside slot 1 (x=4, y=204, w=72, h=28)
static void drawWeaponIcon(int weaponType) {
    // Clear weapon area inside the box
    lcd.fillRect(5, 205, 70, 26, COLOR_BG_PANEL);

    if (weaponType == WEAPON_SMG) {
        // SMG �?compact body, medium barrel, short handle
        lcd.fillRect( 8, 210, 24,  6, COLOR_LIGHTGRAY); // body
        lcd.fillRect(32, 211, 14,  4, COLOR_LIGHTGRAY); // barrel (medium)
        lcd.fillRect(14, 216,  6,  9, COLOR_GRAY);       // grip/handle
        lcd.drawText(20, 221, "SMG", COLOR_CYAN, COLOR_BG_PANEL, 1);
    } else if (weaponType == WEAPON_MG) {
        // MG �?big body, long barrel, bipod legs
        lcd.fillRect( 6, 209, 26,  8, COLOR_LIGHTGRAY); // body (wider)
        lcd.fillRect(32, 210, 22,  6, COLOR_LIGHTGRAY); // long barrel
        lcd.fillRect(54, 211,  4,  4, COLOR_GRAY);       // muzzle tip
        lcd.fillRect(12, 217,  8,  9, COLOR_GRAY);       // grip/handle
        lcd.fillRect(48, 215,  2,  9, COLOR_GRAY);       // bipod left
        lcd.fillRect(54, 215,  2,  9, COLOR_GRAY);       // bipod right
        lcd.drawText(22, 221, "MG", COLOR_ORANGE, COLOR_BG_PANEL, 1);
    } else if (weaponType == WEAPON_LASER) {
        // Laser Sword - handle, guard, long energy blade
        lcd.fillRect(10, 212, 10,  4, COLOR_GRAY);       // handle
        lcd.fillRect(20, 209,  4, 10, COLOR_LIGHTGRAY);  // crossguard
        lcd.fillRect(24, 211, 40,  6, COLOR_CYAN);       // energy blade
        lcd.fillRect(26, 212, 36,  4, COLOR_WHITE);      // blade core
        lcd.drawText(18, 222, "L-SWD", COLOR_CYAN, COLOR_BG_PANEL, 1);
    } else {
        // Laser Gun - high-tech body, long barrel
        lcd.fillRect( 8, 209, 20,  8, COLOR_LIGHTGRAY); // body
        lcd.fillRect(28, 211, 26,  4, COLOR_CYAN);      // barrel
        lcd.fillRect(12, 217,  8,  9, COLOR_GRAY);      // grip/handle
        lcd.fillRect(20, 207,  6,  4, COLOR_GRAY);      // scope
        lcd.drawText(18, 222, "L-GUN", COLOR_CYAN, COLOR_BG_PANEL, 1);
    }
}

// Only redraw slot 1 (weapon) �?no fillRect on the whole bar, no flicker
void drawWeaponSlot(int weaponType) {
    // Clear just the slot area including the border pixel
    lcd.fillRect(4, 204, 72, 28, COLOR_BG_PANEL);
    lcd.drawRect(4, 204, 72, 28, COLOR_WHITE);
    drawWeaponIcon(weaponType);
}

// Helper: draw two-line area name into slot 3 interior (no border repaint)
static void drawAreaName(int x, int y) {
    const char* l1;
    const char* l2;
    switch (s_areaId) {
        case AREA_HOME:       l1 = "ASHROOT"; l2 = "FARM";    break;
        case AREA_TRADE:      l1 = "DAWN";    l2 = "SUPPLY";  break;
        case AREA_INDUSTRIAL: l1 = "RUST";    l2 = "FORGE";   break;
        case AREA_FARMLAND:   l1 = "ASHVEIL"; l2 = "LANDS";   break;
        case AREA_RANDOM:     l1 = "LOST";    l2 = "EXPANSE"; break;
        case AREA_BASE:       l1 = "ECLIPSE"; l2 = "BASE";    break;
        case AREA_ALTAR:      l1 = "RELIC";   l2 = "ALTAR";   break;
        default:              l1 = "UNKNOWN"; l2 = "ZONE";    break;
    }
    lcd.drawText(x, y,      l1, COLOR_CYAN,      COLOR_BG_PANEL, 1);
    lcd.drawText(x, y + 11, l2, COLOR_LIGHTGRAY, COLOR_BG_PANEL, 1);
}

// Redraw the bottom action bar �?weapon slot, backpack slot, map name slot
void drawBottomBar(int weaponType) {
    lcd.fillRect(0, 200, 240, 40, COLOR_BG_PANEL);

    // --- Slot 1: current weapon ---
    lcd.drawRect(4, 204, 72, 28, COLOR_WHITE);
    drawWeaponIcon(weaponType);

    // --- Slot 2: backpack ---
    lcd.drawRect(84, 204, 72, 28, COLOR_WHITE);
    // Bag body
    lcd.fillRect(108, 208, 20, 18, COLOR_GRAY);
    lcd.drawRect(108, 208, 20, 18, COLOR_LIGHTGRAY);
    // Top straps
    lcd.fillRect(109, 205,  2,  5, COLOR_GRAY);
    lcd.fillRect(125, 205,  2,  5, COLOR_GRAY);
    // Top flap
    lcd.fillRect(110, 205, 16,  4, COLOR_DARKGRAY);
    // Buckle strap
    lcd.fillRect(115, 216,  6,  2, COLOR_YELLOW);
    // Label
    lcd.drawText(90, 221, "PACK", COLOR_LIGHTGRAY, COLOR_BG_PANEL, 1);

    // --- Slot 3: current map name ---
    lcd.drawRect(164, 204, 72, 28, COLOR_WHITE);
    drawAreaName(172, 210);
}

// Draw a small colored icon in the center of a tile for a dropped item
static void drawDropIcon(int tileX, int tileY, int itemType) {
    int px = PLAY_AREA_X + tileX * TILE_SIZE + 5 - s_cameraX;
    int py = PLAY_AREA_Y + tileY * TILE_SIZE + 5;

    // clip if completely off screen
    if (px <= PLAY_AREA_X - TILE_SIZE || px >= PLAY_AREA_X + PLAY_AREA_W) return;

    // Medkit: first-aid kit style �?red body, white border, white cross
    if (itemType == INV_MEDKIT) {
        // Red filled box (8x8 for better visibility)
        int mx = PLAY_AREA_X + tileX * TILE_SIZE + 4 - s_cameraX;
        int my = PLAY_AREA_Y + tileY * TILE_SIZE + 4;
        lcd.fillRect(mx, my, 8, 8, COLOR_RED);         // red body
        lcd.drawRect(mx, my, 8, 8, COLOR_WHITE);       // white border
        // White cross in center (3 wide, 6 tall vertical + 6 wide, 2 tall horizontal)
        lcd.fillRect(mx+3, my+1, 2, 6, COLOR_WHITE);   // vertical bar of cross
        lcd.fillRect(mx+1, my+3, 6, 2, COLOR_WHITE);   // horizontal bar of cross
        return;
    }
    uint16_t color;
    switch (itemType) {
        case INV_SEEDS:        color = COLOR_GREEN;      break;
        case INV_FOOD:         color = COLOR_ORANGE;     break;
        case INV_METAL:        color = COLOR_LIGHTGRAY;  break;
        case INV_ALIEN_RELICS: color = COLOR_MAGENTA;    break;
        default:               color = COLOR_WHITE;      break;
    }
    lcd.fillRect(px, py, 6, 6, color);
    lcd.drawRect(px, py, 6, 6, COLOR_WHITE);
}

// Draw all active drops on the map
static void drawAllDrops() {
    for (int i = 0; i < MAX_DROPS; i++) {
        if (drops[i].active) {
            drawDropIcon(drops[i].tileX, drops[i].tileY, drops[i].itemType);
        }
    }
}

// ── Area-specific wall (tile 1) ────────────────────────────────────
static void drawWall(int px, int py, int tx, int ty) {
    switch (s_areaId) {

    case AREA_FARMLAND: {
        // Wooden barn plank / fence post
        lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x8220); // warm brown base
        // Vertical wood-grain lines
        lcd.drawFastVLine(px + 3,  py, TILE_SIZE, 0x4120);
        lcd.drawFastVLine(px + 8,  py, TILE_SIZE, 0x4120);
        lcd.drawFastVLine(px + 12, py, TILE_SIZE, 0x4120);
        // Horizontal plank seam
        lcd.drawFastHLine(px, py + 7, TILE_SIZE, 0x3900);
        // Knot hole (deterministic)
        if (((tx * 7 + ty * 5) & 0x3) == 0) {
            lcd.fillRect(px + 5, py + 2, 3, 3, 0x3900);
        }
        if (((tx * 3 + ty * 9) & 0x3) == 1) {
            lcd.fillRect(px + 9, py + 10, 2, 2, 0x3900);
        }
        // Edge shadow for depth
        lcd.drawFastVLine(px + 15, py, TILE_SIZE, 0x2900);
        lcd.drawFastHLine(px, py + 15, TILE_SIZE, 0x2900);
        break;
    }

    case AREA_RANDOM: {
        // Crumbling concrete block
        lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x6B6D); // medium gray
        // Mortar lines
        lcd.drawFastHLine(px, py + 7,  TILE_SIZE, 0x39E7);
        lcd.drawFastVLine(px + 7, py,  8,         0x39E7);
        lcd.drawFastVLine(px + 7, py + 9, 7,      0x4228);
        // Crack pattern (deterministic per tile)
        if (((tx * 5 + ty * 3) & 0x3) == 0) {
            lcd.drawFastHLine(px + 2, py + 3, 5, 0x2104);
            lcd.drawFastVLine(px + 6, py + 3, 4, 0x2104);
        }
        if (((tx * 9 + ty * 7) & 0x7) == 2) {
            lcd.drawFastHLine(px + 8, py + 11, 6, 0x2104);
        }
        // Aggregate pebble flecks
        if (((tx + ty * 11) & 0x7) < 3) {
            lcd.drawPixel(px + 4, py + 12, 0x9CF3);
            lcd.drawPixel(px + 11, py + 5, 0x9CF3);
        }
        // Shadow edges
        lcd.drawFastVLine(px + 15, py, TILE_SIZE, 0x2104);
        lcd.drawFastHLine(px, py + 15, TILE_SIZE, 0x2104);
        break;
    }

    case AREA_BASE: {
        // Alien alloy bulkhead �?dark teal with hex grid and cyan glow
        lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x0208); // near-black teal
        // Hexagonal grid lines (simplified as crosshatch)
        lcd.drawFastHLine(px, py + 5,  TILE_SIZE, 0x0330);
        lcd.drawFastHLine(px, py + 11, TILE_SIZE, 0x0330);
        lcd.drawFastVLine(px + 5,  py, TILE_SIZE, 0x0330);
        lcd.drawFastVLine(px + 11, py, TILE_SIZE, 0x0330);
        // Glowing cyan circuit node at intersections
        lcd.drawPixel(px + 5,  py + 5,  0x07FF);
        lcd.drawPixel(px + 11, py + 5,  0x07FF);
        lcd.drawPixel(px + 5,  py + 11, 0x07FF);
        lcd.drawPixel(px + 11, py + 11, 0x07FF);
        // Pulsing energy glow on top edge
        lcd.drawFastHLine(px, py, TILE_SIZE, 0x041F);
        // Right shadow
        lcd.drawFastVLine(px + 15, py, TILE_SIZE, 0x0004);
        break;
    }

    default: {
        // AREA_INDUSTRIAL �?corroded iron plate (original Rustforge look)
        lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x4228);
        lcd.drawFastHLine(px, py + 5,  TILE_SIZE, 0x2104);
        lcd.drawFastHLine(px, py + 11, TILE_SIZE, 0x2104);
        lcd.fillRect(px + 1,  py + 4,  2, 2, 0x8410);
        lcd.fillRect(px + 13, py + 4,  2, 2, 0x8410);
        lcd.fillRect(px + 1,  py + 10, 2, 2, 0x8410);
        lcd.fillRect(px + 13, py + 10, 2, 2, 0x8410);
        if (((tx * 5 + ty * 11) & 0x3) == 0)
            lcd.fillRect(px + 2,  py + 1, 4, 3, 0xA300);
        if (((tx * 11 + ty * 7) & 0x7) == 2)
            lcd.fillRect(px + 10, py + 6, 4, 4, 0x8240);
        if (((tx * 3 + ty * 13) & 0x7) == 5)
            lcd.fillRect(px + 5,  py + 12, 5, 3, 0x8240);
        lcd.drawFastHLine(px, py,      TILE_SIZE, 0xA940);
        lcd.drawFastVLine(px + 15, py, TILE_SIZE, 0x2104);
        lcd.drawFastHLine(px, py + 15, TILE_SIZE, 0x2104);
        break;
    }
    }
}

// ── Area-specific thematic passable ground (tile 2) ────────────────
static void drawThematic(int px, int py, int tx, int ty) {
    switch (s_areaId) {

    case AREA_FARMLAND: {
        // Dead crop furrow �?dusty ash-grey soil with withered row lines
        lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x4A26); // dusty olive-gray
        // Crop row furrow lines (horizontal)
        lcd.drawFastHLine(px, py + 4,  TILE_SIZE, 0x3165);
        lcd.drawFastHLine(px, py + 12, TILE_SIZE, 0x3165);
        // Withered stalk stub (deterministic)
        if (((tx * 7 + ty * 5) & 0x3) == 0) {
            lcd.drawFastVLine(px + 4, py + 1, 3, 0x8240);
            lcd.drawPixel(px + 3, py + 1, 0x8240); // leaf nub
        }
        if (((tx * 3 + ty * 11) & 0x3) == 1) {
            lcd.drawFastVLine(px + 11, py + 6, 3, 0x8240);
        }
        // Ash dust fleck
        lcd.drawPixel(px + 7, py + 8, 0x9CF3);
        break;
    }

    case AREA_RANDOM: {
        // Toxic puddle �?murky green-gray with sickly glow patches
        lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x2863); // dark murky green
        // Toxic shimmer lines
        lcd.drawFastHLine(px + 1, py + 4,  8, 0x2FE0);
        lcd.drawFastHLine(px + 3, py + 11, 6, 0x1FC0);
        // Sickly glow spots
        if (((tx * 7 + ty * 3) & 0x3) == 0) {
            lcd.fillRect(px + 2, py + 6, 3, 3, 0x3FE0);  // bright toxic green
        }
        if (((tx * 3 + ty * 7) & 0x7) == 3) {
            lcd.drawPixel(px + 12, py + 3, 0x7FE0);
            lcd.drawPixel(px + 13, py + 4, 0x7FE0);
        }
        // Edge foam
        lcd.drawFastHLine(px, py, TILE_SIZE, 0x1780);
        break;
    }

    case AREA_BASE: {
        // Alien biofloor �?void black with purple/magenta bioluminescence
        lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x0008); // near-black
        // Bio-luminescent vein network
        lcd.drawFastHLine(px + 1, py + 5,  6, 0x8010);
        lcd.drawFastHLine(px + 7, py + 10, 5, 0x8010);
        lcd.drawFastVLine(px + 8, py + 1,  5, 0x8010);
        // Bright glow nodes (deterministic)
        if (((tx * 7 + ty * 3) & 0x3) == 0) {
            lcd.fillRect(px + 2,  py + 2, 2, 2, 0xF81F);  // magenta pulse
        }
        if (((tx * 3 + ty * 7) & 0x7) == 3) {
            lcd.drawPixel(px + 12, py + 9, 0xF81F);
            lcd.drawPixel(px + 11, py + 8, 0xA00C);
        }
        if (((tx + ty * 5) & 0x7) == 2) {
            lcd.fillRect(px + 9, py + 3, 2, 2, 0x600C); // dim purple blob
        }
        break;
    }

    default: {
        // AREA_INDUSTRIAL �?scorched forge ground (original look)
        lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x3820);
        lcd.drawFastHLine(px + 1, py + 5,  7, 0x2800);
        lcd.drawFastHLine(px + 7, py + 11, 5, 0x2800);
        if ((tx + ty) & 1)
            lcd.drawFastVLine(px + 9, py + 2, 6, 0x2800);
        if (((tx * 7 + ty * 3) & 0x3) == 0)
            lcd.fillRect(px + 2,  py + 2, 4, 4, 0xA940);
        if (((tx * 3 + ty * 7) & 0x7) == 3)
            lcd.fillRect(px + 10, py + 9, 2, 2, 0xFD20);
        if (((tx + ty * 3) & 0x7) == 5)
            lcd.fillRect(px + 12, py + 3, 2, 3, 0xB920);
        lcd.drawFastHLine(px, py, TILE_SIZE, 0x5A00);
        break;
    }
    }
}

static void drawGroundDecorBase(int px, int py, int tx, int ty) {
    switch (s_areaId) {
        case AREA_INDUSTRIAL: {
            int bg_x = (tx * TILE_SIZE) % FACTORY_BG_WIDTH;
            int bg_y = (ty * TILE_SIZE) % FACTORY_BG_HEIGHT;
            lcd.drawSubBitmapTransparent(px, py, TILE_SIZE, TILE_SIZE, &factory_bg[bg_y * FACTORY_BG_WIDTH + bg_x], FACTORY_BG_WIDTH);


            break;
        }

        case AREA_FARMLAND: {
            int bg_x = (tx * TILE_SIZE) % FARM_BG_WIDTH;
            int bg_y = (ty * TILE_SIZE) % FARM_BG_HEIGHT;
            lcd.drawSubBitmapTransparent(px, py, TILE_SIZE, TILE_SIZE, &farm_bg[bg_y * FARM_BG_WIDTH + bg_x], FARM_BG_WIDTH);
            break;
        }

        case AREA_RANDOM: {
            int bg_x = (tx * TILE_SIZE) % LOST_BG_WIDTH;
            int bg_y = (ty * TILE_SIZE) % LOST_BG_HEIGHT;
            lcd.drawSubBitmapTransparent(px, py, TILE_SIZE, TILE_SIZE, &lost_bg[bg_y * LOST_BG_WIDTH + bg_x], LOST_BG_WIDTH);
            break;
        }

        case AREA_BASE: {
            int bg_x = (tx * TILE_SIZE) % BASE_BG_WIDTH;
            int bg_y = (ty * TILE_SIZE) % BASE_BG_HEIGHT;
            lcd.drawSubBitmapTransparent(px, py, TILE_SIZE, TILE_SIZE, &base_bg[bg_y * BASE_BG_WIDTH + bg_x], BASE_BG_WIDTH);


            break;
        }

        case AREA_HOME:
            lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x4A48);
            lcd.drawFastHLine(px, py + 12, TILE_SIZE, 0x39A6);
            if (((tx + ty) & 1) == 0) lcd.drawFastVLine(px + 6, py + 3, 6, 0x52A8);
            lcd.drawPixel(px + 11, py + 5, 0x6B6D);
            break;

        case AREA_TRADE:
            supplyDrawGround(px, py, tx, ty);
            break;

        case AREA_ALTAR:
            altarDrawGround(px, py, tx, ty);
            break;

        default:
            lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x2104);
            lcd.drawFastHLine(px + 1, py + 5, 7, 0x4208);
            lcd.drawFastHLine(px + 7, py + 11, 5, 0x4208);
            lcd.drawPixel(px + 12, py + 3, 0x7BEF);
            break;
    }
}

static void drawDecorIcon(int px, int py, int tx, int ty) {
    drawGroundDecorBase(px, py, tx, ty);
    switch (s_areaId) {
        case AREA_INDUSTRIAL:
            if (tx == 6 && ty == 2) {
                lcd.drawPixel(px + 7, py + 3, 0x7BEF);
                lcd.drawPixel(px + 9, py + 2, 0x7BEF);
                lcd.drawPixel(px + 6, py + 5, 0xA514);
                lcd.drawPixel(px + 10, py + 4, 0xA514);
            } else if (tx == 6 && ty == 3) {
                lcd.drawPixel(px + 8, py + 3, 0x7BEF);
                lcd.drawPixel(px + 7, py + 5, 0xA514);
                lcd.drawPixel(px + 9, py + 6, 0xA514);
            } else if ((tx == 12 && ty == 2) || (tx == 11 && ty == 5) || (tx == 8 && ty == 7)) {
                lcd.fillRect(px + 4, py + 9, 8, 2, 0x734B);
                lcd.drawPixel(px + 5, py + 7, COLOR_ORANGE);
                lcd.drawPixel(px + 10, py + 6, COLOR_YELLOW);
            } else {
                lcd.fillRect(px + 5, py + 8, 6, 2, 0x734B);
                lcd.drawPixel(px + 4, py + 10, 0xA300);
                lcd.drawPixel(px + 11, py + 8, 0xA300);
            }
            break;

        case AREA_FARMLAND:
            if ((tx >= 6 && tx <= 8) && (ty >= 6 && ty <= 8)) {
                lcd.drawFastVLine(px + 5, py + 5, 6, 0x6A04);
                lcd.drawFastVLine(px + 8, py + 4, 7, 0x6A04);
                lcd.drawFastVLine(px + 11, py + 6, 5, 0x6A04);
                lcd.drawFastHLine(px + 4, py + 11, 8, 0xA534);
            } else if ((tx == 12 && ty == 2) || (tx == 2 && ty == 5) || (tx == 1 && ty == 7) || (tx == 10 && ty == 8)) {
                lcd.fillRect(px + 5, py + 9, 6, 2, 0xBDF7);
                lcd.drawPixel(px + 6, py + 8, 0xC618);
                lcd.drawPixel(px + 9, py + 8, 0xC618);
            } else {
                lcd.drawFastVLine(px + 8, py + 5, 5, 0x6A04);
                lcd.drawFastHLine(px + 5, py + 10, 6, 0x7386);
            }
            break;

        case AREA_RANDOM:
            if ((tx == 5 && ty == 2) || (tx == 8 && ty == 2) || (tx == 1 && ty == 4) || (tx == 10 && ty == 7)) {
                lcd.fillRect(px + 7, py + 4, 2, 8, 0x05E0);
                lcd.drawFastHLine(px + 5, py + 7, 6, 0x05E0);
                lcd.drawFastHLine(px + 6, py + 10, 4, 0x05E0);
            } else if (tx == 12 && ty == 3) {
                lcd.drawFastVLine(px + 8, py + 4, 8, 0xC3C7);
                lcd.drawFastHLine(px + 5, py + 5, 6, COLOR_RED);
            } else {
                lcd.fillRect(px + 5, py + 8, 6, 2, COLOR_DARKGRAY);
                lcd.drawFastVLine(px + 8, py + 5, 3, COLOR_GRAY);
            }
            break;

        case AREA_BASE:
            // Non-blocking layer: runway lights / warning marks only.
            lcd.drawFastHLine(px + 2, py + 8, 12, 0xB596);
            lcd.drawPixel(px + 4,  py + 8, 0x07FF);
            lcd.drawPixel(px + 11, py + 8, 0x07FF);
            if (((tx + ty) & 1) == 0) {
                lcd.drawPixel(px + 7, py + 5, COLOR_RED);
            }
            break;

        case AREA_ALTAR:
            altarDrawDecor(px, py, tx, ty);
            break;

        case AREA_TRADE:
            supplyDrawDecor(px, py, tx, ty);
            break;

        default:
            lcd.drawFastVLine(px + 8, py + 4, 7, 0x6A04);
            lcd.drawFastHLine(px + 4, py + 7, 8, 0x7BCF);
            break;
    }
}

static void drawBlockerIcon(int px, int py, int tx, int ty) {
    drawGroundDecorBase(px, py, tx, ty);
    switch (s_areaId) {
        case AREA_INDUSTRIAL:
            if (ty == MAP_ROWS - 1) { // Bottom row floor
                lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x3186); // Dark blue-grey floor
                lcd.drawFastHLine(px, py + 2, TILE_SIZE, 0x10A2); // Darker line
                lcd.drawFastHLine(px, py + 14, TILE_SIZE, 0x10A2);
                if (tx % 3 == 0) lcd.drawFastVLine(px + 4, py, TILE_SIZE, 0x10A2);
                return;
            }
            if (tx >= 3 && tx <= 4 && ty >= 1 && ty <= 2) {
                int lx = tx - 3;
                int ly = ty - 1;
                lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x39E7);
                if (ly == 0) lcd.fillRect(px + 2, py + 2, 12, 3, 0x7BEF);
                if (ly == 1) lcd.fillRect(px + 2, py + 4, 12, 8, 0x5AEB);
                if (lx == 0) lcd.fillRect(px + 2, py + 4, 3, 8, 0x4208);
                if (lx == 1) lcd.fillRect(px + 11, py + 4, 3, 8, 0x4208);
                if (ly == 1) lcd.fillRect(px + 4, py + 7, 8, 2, 0xFD20);
                if (lx == 1 && ly == 0) lcd.fillRect(px + 5, py, 3, 4, 0x7BEF);
                if (lx == 1 && ly == 0) {
                    lcd.drawPixel(px + 6, py - 1, 0x8410);
                    lcd.drawPixel(px + 7, py + 1, 0x8410);
                    lcd.drawPixel(px + 9, py + 2, 0x8410);
                }
            } else if (tx >= 9 && tx <= 10 && ty >= 1 && ty <= 2) {
                int lx = tx - 9;
                int ly = ty - 1;
                lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x5AEB);
                if (ly == 0) lcd.fillRect(px + 1, py + 4, 14, 3, 0x94B2);
                if (ly == 1) lcd.fillRect(px + 1, py + 7, 14, 5, 0x4208);
                if (lx == 0) lcd.drawFastVLine(px + 3, py + 5, 6, 0xA534);
                if (lx == 1) lcd.drawFastVLine(px + 12, py + 5, 6, 0xA534);
            } else if (tx >= 2 && tx <= 3 && ty >= 6 && ty <= 7) {
                int lx = tx - 2;
                int ly = ty - 6;
                lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x6B4D);
                if (ly == 0) lcd.fillRect(px + 2, py + 5, 12, 3, 0x9CF3);
                if (ly == 1) lcd.fillRect(px + 2, py + 8, 12, 3, 0x9CF3);
                if (lx == 0) lcd.fillRect(px + 3, py + 5, 4, 6, 0x4208);
                if (lx == 1) lcd.fillRect(px + 9, py + 5, 4, 6, 0x4208);
            } else {
                lcd.fillRect(px, py + 3, 16, 10, 0x52AA); // Iron grey
                lcd.drawFastHLine(px, py + 3, 16, 0x9CD3); // Highlight
                lcd.drawFastHLine(px, py + 12, 16, 0x2945); // Shadow
                lcd.drawFastVLine(px + 4, py + 3, 10, 0x2945); // Rivet line
                lcd.drawFastVLine(px + 12, py + 3, 10, 0x2945); // Rivet line
                lcd.fillRect(px + 6, py + 6, 4, 4, 0xFD20); // Warning orange
            }
            break;

        case AREA_FARMLAND:
            if (ty == MAP_ROWS - 1) { // Bottom row floor
                lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x4A66); // Brown dirt floor
                lcd.drawFastHLine(px, py + 2, TILE_SIZE, 0x2104);
                lcd.drawFastHLine(px, py + 14, TILE_SIZE, 0x2104);
                if (tx % 3 == 0) lcd.drawFastVLine(px + 4, py, TILE_SIZE, 0x2104);
                return;
            }
            if (tx >= 3 && tx <= 4 && ty >= 1 && ty <= 2) {
                int lx = tx - 3;
                int ly = ty - 1;
                if (ly == 0) lcd.fillRect(px, py + 3, 16, 4, 0xA2A5);
                if (ly == 1) lcd.fillRect(px + 1, py + 6, 14, 7, 0x8A62);
                if (lx == 0 && ly == 1) lcd.fillRect(px + 10, py + 8, 6, 5, 0x5A08);
                if (lx == 1 && ly == 1) lcd.fillRect(px, py + 8, 6, 5, 0x5A08);
                if (lx == 0) lcd.drawFastVLine(px + 2, py + 6, 7, 0x6A04);
                if (lx == 1) lcd.drawFastVLine(px + 13, py + 6, 7, 0x6A04);
            } else if (tx == 10 && (ty == 1 || ty == 2)) {
                lcd.fillRect(px + 5, py + 2, 6, 11, 0xC618);
                lcd.drawRect(px + 5, py + 2, 6, 11, COLOR_LIGHTGRAY);
                if (ty == 1) lcd.drawFastHLine(px + 6, py + 2, 4, 0x7BEF);
            } else {
                lcd.fillRect(px, py + 3, 16, 10, 0x4A49); // Dirt/Wood brown
                lcd.drawFastHLine(px, py + 3, 16, 0x2584); // Grass top
                lcd.drawFastHLine(px, py + 4, 16, 0x2584); // Grass top
                lcd.drawFastHLine(px, py + 12, 16, 0x2104); // Bottom shadow
                lcd.drawPixel(px + 2, py + 5, 0x2584); // Hanging grass
                lcd.drawPixel(px + 7, py + 5, 0x2584);
                lcd.drawPixel(px + 13, py + 6, 0x2584);
            }
            break;

        case AREA_RANDOM:
            if (ty == MAP_ROWS - 1) { // Bottom row floor
                lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x6B6D); // Concrete gray floor
                lcd.drawFastHLine(px, py + 2, TILE_SIZE, 0x39E7);
                lcd.drawFastHLine(px, py + 14, TILE_SIZE, 0x39E7);
                if (tx % 3 == 0) lcd.drawFastVLine(px + 4, py, TILE_SIZE, 0x39E7);
                return;
            }
            if (tx >= 9 && tx <= 10 && ty >= 1 && ty <= 2) {
                int lx = tx - 9;
                int ly = ty - 1;
                if (ly == 0) lcd.fillRect(px, py + 4, 16, 4, 0x630C);
                if (ly == 1) lcd.fillRect(px + 1, py + 8, 14, 5, 0x630C);
                if (lx == 0 && ly == 1) lcd.fillRect(px + 10, py + 9, 6, 4, 0x4208);
                if (lx == 1 && ly == 1) lcd.fillRect(px, py + 9, 6, 4, 0x4208);
                if (ly == 0) lcd.drawFastHLine(px + 2, py + 4, 12, 0x9CF3);
            } else if (tx >= 3 && tx <= 4 && ty >= 4 && ty <= 5) {
                int lx = tx - 3;
                int ly = ty - 4;
                if (ly == 0) lcd.fillRect(px, py + 4, 16, 4, 0x7BEF);
                if (ly == 1) lcd.fillRect(px + 1, py + 8, 14, 5, 0x7BEF);
                if (lx == 0 && ly == 1) lcd.fillRect(px + 10, py + 9, 6, 4, 0x4208);
                if (lx == 1 && ly == 1) lcd.fillRect(px, py + 9, 6, 4, 0x4208);
                if (ly == 0) lcd.drawFastHLine(px + 2, py + 4, 12, 0xC618);
            } else {
                lcd.fillRect(px + 2, py + 8, 12, 3, COLOR_DARKGRAY);
                lcd.fillRect(px + 4, py + 6, 8, 2, COLOR_GRAY);
                lcd.drawPixel(px + 4, py + 11, 0x2104);
                lcd.drawPixel(px + 11, py + 11, 0x2104);
            }
            break;

        case AREA_BASE:
            if (ty == MAP_ROWS - 1) { // Bottom row floor
                lcd.fillRect(px, py, TILE_SIZE, TILE_SIZE, 0x0208); // Near-black teal floor
                lcd.drawFastHLine(px, py + 2, TILE_SIZE, 0x0330);
                lcd.drawFastHLine(px, py + 14, TILE_SIZE, 0x0330);
                if (tx % 3 == 0) lcd.drawFastVLine(px + 4, py, TILE_SIZE, 0x0330);
                return;
            }
            if (tx >= 2 && tx <= 4 && ty >= 1 && ty <= 4) {
                int lx = tx - 2;
                int ly = ty - 1;
                if (ly == 0) {
                    if (lx == 0) lcd.fillRect(px + 12, py + 8, 4, 6, COLOR_WHITE);
                    if (lx == 1) {
                        lcd.fillRect(px + 2, py + 4, 12, 10, COLOR_WHITE);
                        lcd.fillRect(px + 6, py + 1, 4, 3, COLOR_RED);
                    }
                    if (lx == 2) lcd.fillRect(px, py + 8, 4, 6, COLOR_WHITE);
                } else if (ly == 1 || ly == 2) {
                    if (lx == 0) lcd.fillRect(px + 10, py, 6, 16, COLOR_WHITE);
                    if (lx == 1) {
                        lcd.fillRect(px + 2, py, 12, 16, COLOR_WHITE);
                        lcd.fillRect(px + 2, py + 4, 12, 2, COLOR_RED);
                        if (ly == 1) lcd.fillRect(px + 6, py + 9, 4, 4, 0x07FF);
                        if (ly == 2) lcd.fillRect(px + 6, py + 3, 4, 4, 0x07FF);
                    }
                    if (lx == 2) lcd.fillRect(px, py, 6, 16, COLOR_WHITE);
                } else {
                    if (lx == 0) {
                        lcd.fillRect(px + 10, py, 6, 10, COLOR_WHITE);
                        lcd.fillRect(px + 10, py + 10, 6, 4, COLOR_RED);
                    }
                    if (lx == 1) {
                        lcd.fillRect(px + 2, py, 12, 10, COLOR_WHITE);
                        lcd.fillRect(px + 5, py + 10, 6, 4, COLOR_ORANGE);
                    }
                    if (lx == 2) {
                        lcd.fillRect(px, py, 6, 10, COLOR_WHITE);
                        lcd.fillRect(px, py + 10, 6, 4, COLOR_RED);
                    }
                }
            } else if (tx >= 9 && tx <= 10 && ty >= 2 && ty <= 3) {
                int lx = tx - 9;
                int ly = ty - 2;
                if (ly == 0) {
                    if (lx == 0) {
                        lcd.fillRect(px + 2, py + 10, 14, 4, COLOR_WHITE);
                        lcd.fillRect(px + 9, py + 6, 4, 4, COLOR_WHITE);
                    } else {
                        lcd.fillRect(px, py + 10, 14, 4, COLOR_WHITE);
                        lcd.fillRect(px + 3, py + 6, 4, 4, COLOR_WHITE);
                        lcd.fillRect(px + 10, py + 8, 3, 2, 0x07FF);
                    }
                } else {
                    if (lx == 0) {
                        lcd.fillRect(px + 8, py, 4, 10, COLOR_WHITE);
                        lcd.fillRect(px + 4, py + 10, 8, 3, COLOR_WHITE);
                    } else {
                        lcd.fillRect(px + 4, py, 4, 10, COLOR_WHITE);
                        lcd.fillRect(px + 4, py + 10, 8, 3, COLOR_WHITE);
                    }
                }
            } else if (tx >= 9 && tx <= 10 && ty >= 6 && ty <= 7) {
                int lx = tx - 9;
                int ly = ty - 6;
                if (ly == 0) {
                    if (lx == 0) {
                        lcd.fillRect(px + 2, py + 10, 14, 4, COLOR_WHITE);
                        lcd.fillRect(px + 9, py + 6, 4, 4, COLOR_WHITE);
                    } else {
                        lcd.fillRect(px, py + 10, 14, 4, COLOR_WHITE);
                        lcd.fillRect(px + 3, py + 6, 4, 4, COLOR_WHITE);
                        lcd.fillRect(px + 10, py + 8, 3, 2, 0x07FF);
                    }
                } else {
                    if (lx == 0) {
                        lcd.fillRect(px + 8, py, 4, 10, COLOR_WHITE);
                        lcd.fillRect(px + 4, py + 10, 8, 3, COLOR_WHITE);
                    } else {
                        lcd.fillRect(px + 4, py, 4, 10, COLOR_WHITE);
                        lcd.fillRect(px + 4, py + 10, 8, 3, COLOR_WHITE);
                    }
                }
            } else if (tx >= 6 && tx <= 7 && ty >= 8 && ty <= 9) {
                int lx = tx - 6;
                int ly = ty - 8;
                lcd.fillRect(px + 1, py + 5, 14, 8, 0xC618);
                if (ly == 0) {
                    lcd.fillRect(px + 4, py + 2, 8, 3, 0x39E7);
                    lcd.drawFastVLine(px + 8, py, 3, COLOR_RED);
                }
                if (lx == 0) {
                    lcd.fillRect(px + 2, py + 7, 3, 6, 0x9CF3);
                    lcd.fillRect(px + 10, py + 7, 6, 3, 0x07FF);
                } else {
                    lcd.fillRect(px, py + 7, 6, 3, 0x07FF);
                    lcd.fillRect(px + 11, py + 7, 3, 6, 0x9CF3);
                }
                if (ly == 1) {
                    lcd.drawFastHLine(px + 2, py + 12, 12, 0x7BEF);
                }
            } else if (tx >= 11 && tx <= 12 && ty >= 8 && ty <= 9) {
                int lx = tx - 11;
                int ly = ty - 8;
                // New hangar block at bottom-right
                lcd.fillRect(px + 1, py + 5, 14, 8, 0xBDF7);
                if (ly == 0) {
                    lcd.fillRect(px + 2, py + 2, 12, 3, 0x9CF3); // roof line
                    lcd.drawFastHLine(px + 3, py + 6, 10, 0x39E7);
                }
                if (ly == 1) {
                    if (lx == 0) lcd.fillRect(px + 9, py + 7, 7, 6, 0x07FF);
                    if (lx == 1) lcd.fillRect(px, py + 7, 7, 6, 0x07FF);
                    lcd.drawFastHLine(px + 2, py + 12, 12, 0x7BEF);
                }
                if (lx == 0) lcd.drawFastVLine(px + 2, py + 6, 7, 0x7BEF);
                if (lx == 1) lcd.drawFastVLine(px + 13, py + 6, 7, 0x7BEF);
            } else {
                lcd.fillRect(px + 2, py + 6, 12, 5, 0x5AEB);
                lcd.drawRect(px + 2, py + 6, 12, 5, 0xBDF7);
                lcd.fillRect(px + 6, py + 7, 4, 2, 0x07FF);
            }
            break;

        case AREA_ALTAR:
            altarDrawBlocker(px, py, tx, ty);
            break;

        case AREA_TRADE:
            supplyDrawBlocker(px, py, tx, ty);
            break;

        default:
            drawDecorIcon(px, py, tx, ty);
            break;
    }
}

static void drawTile(int tx, int ty, uint8_t tile) {
    int px = PLAY_AREA_X + tx * TILE_SIZE - s_cameraX;
    int py = PLAY_AREA_Y + ty * TILE_SIZE;

    // clip if completely off screen
    if (px <= PLAY_AREA_X - TILE_SIZE || px >= PLAY_AREA_X + PLAY_AREA_W) return;

    if (tile == 1) {
        drawBlockerIcon(px, py, tx, ty);
    } else if (tile == 2) {
        drawDecorIcon(px, py, tx, ty);
    } else {
        drawGroundDecorBase(px, py, tx, ty);
    }
}

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
            lcd.fillRect(barX + 1, barY + 1, fillW, barH - 2, (chargeMs >= 1000) ? COLOR_RED : COLOR_CYAN);
        }
    }
}

static void drawPlayer(Player* p) {
    int px = PLAY_AREA_X + p->pixelX - s_cameraX;
    int py = PLAY_AREA_Y + p->pixelY;
    drawPlayerAndChargeBar(p, px, py, combatScale());
}

// ── Enemy sprite helpers (16×16 bitmaps) ──────────────────────────
// Each row is a uint16_t with MSB = leftmost pixel (col 0).
// Rendered at scale=2 �?16×16 px (safe zones) or scale=3 �?24×24 px
// (combat zones) via the bit-offset table in drawSpriteLayer16.

// Xenomorph body shared by knife + laser aliens: elongated skull,
// hunched shoulders, digitigrade legs. Weapon layer covers right side.
static const uint16_t sAlienBody[16] = {
    0x0780,  // row 0  .....xxxx.......   crown
    0x0FC0,  // row 1  ....xxxxxx......
    0x1FE0,  // row 2  ...xxxxxxxx.....   skull wide
    0x1FE0,  // row 3  ...xxxxxxxx.....   (eyes overlay)
    0x1FE0,  // row 4  ...xxxxxxxx.....
    0x0FC0,  // row 5  ....xxxxxx......   jaw narrows
    0x0780,  // row 6  .....xxxx.......   neck
    0x1FE0,  // row 7  ...xxxxxxxx.....   shoulders
    0x3FF0,  // row 8  ..xxxxxxxxxx....   upper chest
    0x3FF0,  // row 9  ..xxxxxxxxxx....
    0x3FF0,  // row 10 ..xxxxxxxxxx....
    0x1FE0,  // row 11 ...xxxxxxxx.....   waist
    0x1CE0,  // row 12 ...xxx..xxx.....   thighs split
    0x1CE0,  // row 13
    0x0C60,  // row 14 ....xx..xx......   shins
    0x0C60,  // row 15
};

// Four glowing eyes (2×2 blocks) at rows 3-4.
static const uint16_t sAlienEyes[16] = {
    0x0000, 0x0000, 0x0000, 0x0CC0,   // ....xx..xx......
    0x0CC0, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};

// Curved blade held upright on right shoulder.
static const uint16_t sKnifeBlade[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0004,  // row 4  ..............x.   blade tip
    0x000C,  // row 5  .............xx.
    0x000C,  // row 6
    0x0018,  // row 7  ...........xx...   curve
    0x0018,  // row 8
    0x0010,  // row 9  ...........x....   hilt
    0x0010,  // row 10 handle grip
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

// Long sleek laser rifle �?barrel extends past body on right.
static const uint16_t sLaserGun[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000,
    0x003E,  // row 7  ..........xxxxx.   long barrel
    0x003F,  // row 8  ..........xxxxxx   barrel + muzzle
    0x001C,  // row 9  ...........xxx..   under-grip
    0x0010,  // row 10 ...........x....   trigger
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

// Heavy alien body: massive, squat, horned skull, tank silhouette.
static const uint16_t sHeavyBody[16] = {
    0x1818,  // row 0  ...xx...xx......   horn stubs
    0x3FFC,  // row 1  ..xxxxxxxxxxxx..   skull top
    0x7FFE,  // row 2  .xxxxxxxxxxxxxx.
    0x7FFE,  // row 3  (eyes overlay)
    0x7FFE,  // row 4
    0x3FFC,  // row 5  jaw
    0x7FFE,  // row 6  hunched, no real neck
    0xFFFF,  // row 7  full-width shoulder pads
    0xFFFF,  // row 8  chest (hammer overlay)
    0x7FFE,  // row 9
    0x7FFE,  // row 10
    0x3FFC,  // row 11 waist stays wide
    0x381C,  // row 12 ..xxx......xxx..   thick legs
    0x381C,  // row 13
    0x381C,  // row 14
    0x7C3E,  // row 15 .xxxxx..xxxxx...   flared boots
};

// Heavy alien eyes: 2-pixel outer glow + 1-pixel pupil (wide apart).
static const uint16_t sHeavyEyes[16] = {
    0x0000, 0x0000, 0x0000,
    0x1818,  // row 3  ...xx...xx......
    0x0810,  // row 4  ....x....x......
    0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};

// Two-headed vertical war-hammer on right side.
static const uint16_t sHammer[16] = {
    0x0000, 0x0000,
    0x000E,  // row 2  ............xxx.   upper head
    0x000E,  // row 3
    0x0004,  // row 4  .............x..   shaft
    0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
    0x000E,  // row 10 lower head
    0x000E,  // row 11
    0x0000, 0x0000, 0x0000, 0x0000,
};

static const uint8_t sBeastBody[8] = {
    0x00, 0x3C, 0x7E, 0xFF, 0xBD, 0xFF, 0x66, 0x42
};
static const uint8_t sBeastEyes[8] = {
    0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t sBeastClaws[8] = {
    0x00, 0x00, 0x00, 0x81, 0x42, 0x81, 0x00, 0x00
};

// Colours
#define COL_ALIEN_SKIN  0x27E2   // sickly lime-green
#define COL_ALIEN_EYES  0xF800   // glowing red
#define COL_KNIFE       0xC618   // steel silver
#define COL_LASER_GUN   0x07FF   // electric cyan
#define COL_HEAVY_SKIN  0x600F   // dark alien purple
#define COL_HEAVY_EYES  0xFD20   // orange glow
#define COL_HAMMER      0x8410   // dark grey iron
#define COL_BEAST_SKIN  0x7BE0
#define COL_BEAST_EYES  0xF81F
#define COL_BEAST_CLAW  0xFFFF

// Sprite scale selector: combat areas use 3x (24x24) for a larger, more
// detailed appearance; safe areas (Farm / Dawn Supply / Altar) stay at 2x
// (16x16) so fixed layouts like merchants and the altar aren't disturbed.
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

// Map input-bit column c (0..16) to an output-pixel offset for any outSize.
// 16×16 bitmap �?16 px (scale 2, 1:1) or 24 px (scale 3, alternating 1/2-px
// block widths). Mirrors the helper in character.cpp.
static inline int spriteBitOffset(int c, int outSize) { return (c * outSize) >> 4; }

static void drawSpriteLayer(int px, int py,
                             const uint16_t* data, uint16_t color, int scale = 2) {
    int outSize = 8 * scale;            // 16 or 24
    int off     = (outSize - 16) / 2;
    int bx      = px - off;
    int by      = py - off;
    for (int row = 0; row < 16; row++) {
        uint16_t bits = data[row];
        if (!bits) continue;
        int yTop = by + spriteBitOffset(row, outSize);
        int h    = spriteBitOffset(row + 1, outSize) - spriteBitOffset(row, outSize);
        for (int col = 0; col < 16; col++) {
            if ((bits >> (15 - col)) & 1) {
                int xL = bx + spriteBitOffset(col, outSize);
                int w  = spriteBitOffset(col + 1, outSize) - spriteBitOffset(col, outSize);
                lcd.fillRect(xL, yTop, w, h, color);
            }
        }
    }
}

static void drawEnemySprite(int px, int py, int type, int scale = 2) {
    if (type == ENEMY_KNIFE) {
        drawSpriteLayer(px, py, sAlienBody,  COL_ALIEN_SKIN, scale);
        drawSpriteLayer(px, py, sAlienEyes,  COL_ALIEN_EYES, scale);
        drawSpriteLayer(px, py, sKnifeBlade, COL_KNIFE,      scale);
    } else if (type == ENEMY_LASER) {
        drawSpriteLayer(px, py, sAlienBody, COL_ALIEN_SKIN, scale);
        drawSpriteLayer(px, py, sAlienEyes, COL_ALIEN_EYES, scale);
        drawSpriteLayer(px, py, sLaserGun,  COL_LASER_GUN,  scale);
    } else {
        drawSpriteLayer(px, py, sHeavyBody, COL_HEAVY_SKIN, scale);
        drawSpriteLayer(px, py, sHeavyEyes, COL_HEAVY_EYES, scale);
        drawSpriteLayer(px, py, sHammer,    COL_HAMMER,     scale);
    }
}

// Draw all active enemies
static void drawEnemies() {
    int scale = combatScale();
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        int px = PLAY_AREA_X + enemies[i].pixelX - s_cameraX;
        int py = PLAY_AREA_Y + enemies[i].pixelY;
        drawEnemySprite(px, py, enemies[i].type, scale);
    }
}

// Incremental enemy update: only repaint tiles that changed.
// Two-pass to avoid drawing order conflicts when enemies swap tiles.
void drawEnemyUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS]) {
    int scale = combatScale();
    // Pass 1: erase every old enemy tile (restore map tile + any drop)
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active || !enemies[i].moved) continue;
        drawTile(enemies[i].prevX, enemies[i].prevY,
                 mapData[enemies[i].prevY][enemies[i].prevX]);
        if (dropExistsAt(enemies[i].prevX, enemies[i].prevY)) {
            drawDropIcon(enemies[i].prevX, enemies[i].prevY,
                         dropTypeAt(enemies[i].prevX, enemies[i].prevY));
        }
    }
    // Pass 2: draw each enemy at its new position, then clear the flag
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active || !enemies[i].moved) continue;
        int px = PLAY_AREA_X + enemies[i].tileX * TILE_SIZE;
        int py = PLAY_AREA_Y + enemies[i].tileY * TILE_SIZE;
        drawEnemySprite(px, py, enemies[i].type, scale);
        enemies[i].moved = false;
    }
    // Restore portal sprite if an enemy walked through its tile
    drawPortal();
}

// Repaint every map tile intersecting pixel rect (rx,ry,rw,rh). Restores
// ground / blocker / decor art, any drop on those tiles, and the portal
// sprite if present. Clipped to the play area.
//
// Used to refresh the area exposed by a moving sprite: at scale 3 the 24×24
// sprite overlaps up to 9 underlying tiles, so a flat color fill would wipe
// obstacle decorations when the sprite moves away.
static void redrawTileRect(uint8_t mapData[MAP_ROWS][MAP_COLS],
                           int rx, int ry, int rw, int rh) {
    int left   = rx;
    int top    = ry;
    int right  = rx + rw - 1;
    int bottom = ry + rh - 1;

    int playLeft   = PLAY_AREA_X;
    int playTop    = PLAY_AREA_Y;
    int playRight  = PLAY_AREA_X + PLAY_AREA_W - 1;
    int playBottom = PLAY_AREA_Y + PLAY_AREA_H - 1;
    if (left   < playLeft)   left   = playLeft;
    if (top    < playTop)    top    = playTop;
    if (right  > playRight)  right  = playRight;
    if (bottom > playBottom) bottom = playBottom;
    if (left > right || top > bottom) return;

    int tx0 = (left   - PLAY_AREA_X + s_cameraX) / TILE_SIZE;
    int ty0 = (top    - PLAY_AREA_Y) / TILE_SIZE;
    int tx1 = (right  - PLAY_AREA_X + s_cameraX) / TILE_SIZE;
    int ty1 = (bottom - PLAY_AREA_Y) / TILE_SIZE;

    // ensure tile bounds
    if (tx0 < 0) tx0 = 0;
    if (tx1 >= MAP_COLS) tx1 = MAP_COLS - 1;

    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            drawTile(tx, ty, mapData[ty][tx]);
            if (dropExistsAt(tx, ty)) {
                drawDropIcon(tx, ty, dropTypeAt(tx, ty));
            }
            if (portal.active && portal.tileX == tx && portal.tileY == ty) {
                drawPortalTile(tx, ty);
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// drawSmoothEntitiesUpdate
//
// Per-frame pixel-level redraw for the player and every active enemy.
// Each entity stores its previous draw position in lastDrawnPixelX/Y.
//
// Order of operations (flicker-free, obstacle-preserving):
//   1. Compute current pixel position for every entity.
//   2. For every moved entity, repaint map tiles under the union of its old
//      and new sprite rects. This erases the old-position ghost AND restores
//      obstacle / drop / portal art in the halo that the previous frame's
//      sprite had overdrawn.
//   3. Redraw every active entity non-opaque at its current position on top
//      of the freshly restored tiles. Stationary entities whose tile was
//      wiped by a neighbour's halo get repainted too.
//   4. Latch lastDrawn for every entity.
// ─────────────────────────────────────────────────────────────────────────────
void drawProjectilesUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active && !projectiles[i].pendingErase) continue;

        int oldX = projectiles[i].lastDrawnX - s_cameraX;
        int oldY = PLAY_AREA_Y + projectiles[i].lastDrawnY;

        int drawX, drawY, drawW, drawH;
        if (projectiles[i].type == WEAPON_LASER) {
            drawX = PLAY_AREA_X + oldX;
            drawY = oldY;
            drawW = projectiles[i].lastDrawnW;
            drawH = projectiles[i].lastDrawnH;
            // Pad the redraw rect slightly to ensure full erasure
            drawX -= 4; drawY -= 4;
            drawW += 8; drawH += 8;
        } else {
            int eraseSize = 16;
            int eraseRad = eraseSize / 2;
            drawX = PLAY_AREA_X + oldX - eraseRad;
            drawY = oldY - eraseRad;
            drawW = eraseSize;
            drawH = eraseSize;
        }

        // Clip to screen bounds
        if (drawX < 0) { drawW += drawX; drawX = 0; }
        if (drawY < 0) { drawH += drawY; drawY = 0; }
        if (drawX + drawW > 240) drawW = 240 - drawX;
        if (drawY + drawH > 240) drawH = 240 - drawY;

        if (drawW > 0 && drawH > 0) {
            static uint16_t projDirtyBuf[4096];
            bool useBuf = (drawW * drawH <= 4096);
            if (useBuf) {
                lcd.setRenderTarget(projDirtyBuf, drawX, drawY, drawW, drawH);
            }

            redrawTileRect(mapData, drawX, drawY, drawW, drawH);

            // Re-draw any entity that might have been clipped
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (enemies[e].active) {
                    int ex = PLAY_AREA_X + enemies[e].pixelX - s_cameraX;
                    int ey = PLAY_AREA_Y + enemies[e].pixelY;
                    if (ex > drawX - 32 && ex < drawX + drawW + 32 &&
                        ey > drawY - 32 && ey < drawY + drawH + 32) {
                        drawEnemySprite(ex, ey, enemies[e].type, combatScale());
                    }
                }
            }
            int px = PLAY_AREA_X + player->pixelX - s_cameraX;
            int py = PLAY_AREA_Y + player->pixelY;
            if (px > drawX - 32 && px < drawX + drawW + 32 &&
                py > drawY - 32 && py < drawY + drawH + 32) {
                drawPlayerAndChargeBar(player, px, py, combatScale());
            }

            if (useBuf) {
                lcd.setRenderTarget(nullptr, 0, 0, 0, 0);
                lcd.pushImage(drawX, drawY, drawW, drawH, projDirtyBuf);
            }
        }

        if (projectiles[i].active) {
            int newX = projectiles[i].pixelX - s_cameraX;
            int newY = PLAY_AREA_Y + projectiles[i].pixelY;
            if (projectiles[i].type == WEAPON_LASER) {
                float maxRange = 32.0f; // LASER_SWORD_RANGE * 16
                float progress = (maxRange - (float)projectiles[i].range) / maxRange;
                if (progress < 0.0f) progress = 0.0f;
                if (progress > 1.0f) progress = 1.0f;

                float angle = 0.0f;
                int mcx = player->pixelX + 8;
                int mcy = player->pixelY + 8;

                if (projectiles[i].vx > 0) {
                    angle = -1.5f + 3.0f * progress;
                } else if (projectiles[i].vx < 0) {
                    angle = 3.14159f + 1.5f - 3.0f * progress;
                } else if (projectiles[i].vy > 0) {
                    angle = 3.14159f / 2.0f - 1.5f + 3.0f * progress;
                } else if (projectiles[i].vy < 0) {
                    angle = -3.14159f / 2.0f - 1.5f + 3.0f * progress;
                }

                int bladeLen = 10 * combatScale();
                int minMapX = 9999, maxMapX = -9999;
                int minMapY = 9999, maxMapY = -9999;

                for (int d = 0; d < bladeLen; d += 2) {
                    int bx_map = mcx + (int)(cos(angle) * d);
                    int by_map = mcy + (int)(sin(angle) * d);

                    if (bx_map - 2 < minMapX) minMapX = bx_map - 2;
                    if (bx_map + 2 > maxMapX) maxMapX = bx_map + 2;
                    if (by_map - 2 < minMapY) minMapY = by_map - 2;
                    if (by_map + 2 > maxMapY) maxMapY = by_map + 2;

                    int screenX = PLAY_AREA_X + bx_map - s_cameraX;
                    int screenY = PLAY_AREA_Y + by_map;

                    if (screenX >= 0 && screenX < PLAY_AREA_W && screenY >= PLAY_AREA_Y && screenY < PLAY_AREA_Y + PLAY_AREA_H) {
                        lcd.fillRect(screenX - 2, screenY - 2, 4, 4, COLOR_CYAN);
                        lcd.fillRect(screenX - 1, screenY - 1, 2, 2, COLOR_WHITE);
                    }
                }

                projectiles[i].lastDrawnX = minMapX;
                projectiles[i].lastDrawnY = minMapY;
                projectiles[i].lastDrawnW = maxMapX - minMapX;
                projectiles[i].lastDrawnH = maxMapY - minMapY;

                if (projectiles[i].lastDrawnW < 8) projectiles[i].lastDrawnW = 8;
                if (projectiles[i].lastDrawnH < 8) projectiles[i].lastDrawnH = 8;

            } else {
                int newX = projectiles[i].pixelX - s_cameraX;
                int newY = PLAY_AREA_Y + projectiles[i].pixelY;
                if (newX >= 0 && newX < PLAY_AREA_W && newY >= PLAY_AREA_Y && newY < PLAY_AREA_Y + PLAY_AREA_H) {
                    uint16_t color = COLOR_YELLOW;
                    if (projectiles[i].type == WEAPON_MG) color = COLOR_ORANGE;
                    else if (projectiles[i].type == WEAPON_LASER_GUN) color = COLOR_CYAN;
                    lcd.fillRect(PLAY_AREA_X + newX - 1, newY - 1, 4, 4, color);
                }
                projectiles[i].lastDrawnX = projectiles[i].pixelX;
                projectiles[i].lastDrawnY = projectiles[i].pixelY;
            }
        } else if (projectiles[i].pendingErase) {
            projectiles[i].pendingErase = false;
        }
    }
}

void drawSmoothEntitiesUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player) {
    // --- 1. Current pixel positions (Camera offset applied) ---
    int pCurX = PLAY_AREA_X + player->pixelX - s_cameraX;
    int pCurY = PLAY_AREA_Y + player->pixelY;

    // Is the player swinging the laser sword right now?
    // If so, we need to force them to be treated as "moved" so the sword gets redrawn and erased properly.
    uint32_t swingTime = millis() - player->weapon.lastFire;
    bool playerSwinging = (player->weapon.type == WEAPON_LASER && swingTime < 300);
    // Also force a redraw for a few frames after the swing ends to ensure the sword is erased!
    bool playerJustFinishedSwinging = (player->weapon.type == WEAPON_LASER && swingTime >= 300 && swingTime < 450);

    int  eCurX[MAX_ENEMIES];
    int  eCurY[MAX_ENEMIES];
    bool eMoved[MAX_ENEMIES] = {false};
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;

        // Enemies might not have pixelX initialized properly in safe zones yet,
        // so fallback to tile logic if pixelX is 0 and they are at non-zero tile.
        // Better: use pixelX directly, assuming enemy logic maintains it.
        eCurX[i] = PLAY_AREA_X + enemies[i].pixelX - s_cameraX;
        eCurY[i] = PLAY_AREA_Y + enemies[i].pixelY;

        eMoved[i] = (eCurX[i] != enemies[i].lastDrawnPixelX ||
                     eCurY[i] != enemies[i].lastDrawnPixelY);
    }
    static bool wasCharging = false;
    bool isChargingNow = (player->weapon.type == WEAPON_LASER_GUN && player->weapon.isCharging);
    bool chargingStateChanged = (wasCharging != isChargingNow);
    wasCharging = isChargingNow;

    bool pMoved = (pCurX != player->lastDrawnPixelX ||
                   pCurY != player->lastDrawnPixelY ||
                   playerSwinging || playerJustFinishedSwinging ||
                   isChargingNow || chargingStateChanged);

    bool anyMoved = pMoved;
    if (!anyMoved) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active && eMoved[i]) { anyMoved = true; break; }
        }
    }
    if (!anyMoved) return;

    int scale = combatScale();
    int size  = 8 * scale;
    int off   = (size - 16) / 2;

    // --- 2. Erase trailing strips for each moved entity ---
    // We now redraw the union of old and new sprite rects to fully erase the ghost
    // and restore the background under the new position.
    static uint16_t dirtyBuf[4096];

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active || !eMoved[i]) continue;
        int oldSx = enemies[i].lastDrawnPixelX - off;
        int oldSy = enemies[i].lastDrawnPixelY - off;
        int newSx = eCurX[i] - off;
        int newSy = eCurY[i] - off;

        int minX = (oldSx < newSx) ? oldSx : newSx;
        int minY = (oldSy < newSy) ? oldSy : newSy;
        int maxX = (oldSx > newSx) ? oldSx : newSx;
        int maxY = (oldSy > newSy) ? oldSy : newSy;

        int rw = (maxX + size) - minX;
        int rh = (maxY + size) - minY;

        if (minX < PLAY_AREA_X) { rw -= (PLAY_AREA_X - minX); minX = PLAY_AREA_X; }
        if (minY < PLAY_AREA_Y) { rh -= (PLAY_AREA_Y - minY); minY = PLAY_AREA_Y; }
        if (minX + rw > PLAY_AREA_X + PLAY_AREA_W) { rw = PLAY_AREA_X + PLAY_AREA_W - minX; }
        if (minY + rh > PLAY_AREA_Y + PLAY_AREA_H) { rh = PLAY_AREA_Y + PLAY_AREA_H - minY; }
        if (rw <= 0 || rh <= 0) continue;

        if (rw * rh <= 4096) {
            lcd.setRenderTarget(dirtyBuf, minX, minY, rw, rh);
            redrawTileRect(mapData, minX, minY, rw, rh);
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (enemies[e].active) {
                    drawEnemySprite(eCurX[e], eCurY[e], enemies[e].type, scale);
                }
            }
            drawPlayerAndChargeBar(player, pCurX, pCurY, scale);
            lcd.setRenderTarget(nullptr, 0, 0, 0, 0);
            lcd.pushImage(minX, minY, rw, rh, dirtyBuf);
        } else {
            redrawTileRect(mapData, minX, minY, rw, rh);
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (enemies[e].active) {
                    drawEnemySprite(eCurX[e], eCurY[e], enemies[e].type, scale);
                }
            }
            drawPlayerAndChargeBar(player, pCurX, pCurY, scale);
        }
    }

    if (pMoved) {
        int oldSx = player->lastDrawnPixelX - off;
        int oldSy = player->lastDrawnPixelY - off;
        int newSx = pCurX - off;
        int newSy = pCurY - off;

        int minX = (oldSx < newSx) ? oldSx : newSx;
        int minY = (oldSy < newSy) ? oldSy : newSy;
        int maxX = (oldSx > newSx) ? oldSx : newSx;
        int maxY = (oldSy > newSy) ? oldSy : newSy;

        if (player->weapon.type == WEAPON_LASER) {
            int extraW = 12 * scale; // length of sword blade padding
            minX -= extraW;
            maxX += extraW;
        }

        if (player->weapon.type == WEAPON_LASER_GUN) {
            int barPaddingH = 16 * scale; // padding for charge bar above player
            minY -= barPaddingH;
        }

        int rw = (maxX + size) - minX;
        int rh = (maxY + size) - minY;

        if (minX < PLAY_AREA_X) { rw -= (PLAY_AREA_X - minX); minX = PLAY_AREA_X; }
        if (minY < PLAY_AREA_Y) { rh -= (PLAY_AREA_Y - minY); minY = PLAY_AREA_Y; }
        if (minX + rw > PLAY_AREA_X + PLAY_AREA_W) { rw = PLAY_AREA_X + PLAY_AREA_W - minX; }
        if (minY + rh > PLAY_AREA_Y + PLAY_AREA_H) { rh = PLAY_AREA_Y + PLAY_AREA_H - minY; }

        if (rw > 0 && rh > 0) {
            if (rw * rh <= 4096) {
                lcd.setRenderTarget(dirtyBuf, minX, minY, rw, rh);
                redrawTileRect(mapData, minX, minY, rw, rh);
                for (int e = 0; e < MAX_ENEMIES; e++) {
                    if (enemies[e].active) {
                        drawEnemySprite(eCurX[e], eCurY[e], enemies[e].type, scale);
                    }
                }
                drawPlayerAndChargeBar(player, pCurX, pCurY, scale);
                lcd.setRenderTarget(nullptr, 0, 0, 0, 0);
                lcd.pushImage(minX, minY, rw, rh, dirtyBuf);
            } else {
                redrawTileRect(mapData, minX, minY, rw, rh);
                for (int e = 0; e < MAX_ENEMIES; e++) {
                    if (enemies[e].active) {
                        drawEnemySprite(eCurX[e], eCurY[e], enemies[e].type, scale);
                    }
                }
                drawPlayerAndChargeBar(player, pCurX, pCurY, scale);
            }
        }
    }

    // --- 4. Latch positions for next frame ---
    player->lastDrawnPixelX = pCurX;
    player->lastDrawnPixelY = pCurY;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        enemies[i].lastDrawnPixelX = eCurX[i];
        enemies[i].lastDrawnPixelY = eCurY[i];
    }
}

// Redraw only the single tile where an enemy just died.
// Restores the map background, then overlays any drop that landed there.
// No fillRect on the play area �?zero flicker.
void drawEnemyDeath(uint8_t mapData[MAP_ROWS][MAP_COLS], int tileX, int tileY) {
    drawTile(tileX, tileY, mapData[tileY][tileX]);
    if (dropExistsAt(tileX, tileY)) {
        drawDropIcon(tileX, tileY, dropTypeAt(tileX, tileY));
    }
}

void drawPendingEnemyDeaths(uint8_t mapData[MAP_ROWS][MAP_COLS]) {
    int scale = combatScale();
    int size  = 8 * scale;
    int off   = (size - 16) / 2;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].pendingDeathClear) continue;

        // Compute the tiles that the sprite rect overlapped. With scale>2 the
        // sprite is centred on its tile anchor, so the rect extends `off` px
        // past each edge and can cover up to 9 tiles.
        int left   = enemies[i].lastDrawnPixelX - off;
        int top    = enemies[i].lastDrawnPixelY - off;
        int right  = left + size - 1;
        int bottom = top  + size - 1;

        int playLeft   = PLAY_AREA_X;
        int playTop    = PLAY_AREA_Y;
        int playRight  = PLAY_AREA_X + PLAY_AREA_W - 1;
        int playBottom = PLAY_AREA_Y + MAP_ROWS * TILE_SIZE - 1;
        if (left   < playLeft)   left   = playLeft;
        if (top    < playTop)    top    = playTop;
        if (right  > playRight)  right  = playRight;
        if (bottom > playBottom) bottom = playBottom;

        int tx0 = (left   - PLAY_AREA_X + s_cameraX) / TILE_SIZE;
        int ty0 = (top    - PLAY_AREA_Y) / TILE_SIZE;
        int tx1 = (right  - PLAY_AREA_X + s_cameraX) / TILE_SIZE;
        int ty1 = (bottom - PLAY_AREA_Y) / TILE_SIZE;

        if (tx0 < 0) tx0 = 0;
        if (tx1 >= MAP_COLS) tx1 = MAP_COLS - 1;

        for (int ty = ty0; ty <= ty1; ty++) {
            for (int tx = tx0; tx <= tx1; tx++) {
                drawTile(tx, ty, mapData[ty][tx]);
                if (dropExistsAt(tx, ty)) {
                    drawDropIcon(tx, ty, dropTypeAt(tx, ty));
                }
                if (portal.active && portal.tileX == tx && portal.tileY == ty) {
                    drawPortalTile(tx, ty);
                }
            }
        }
        enemies[i].pendingDeathClear = false;
    }
}

// Draw a bullet trail flash between player tile and target tile, then erase.
// Uses Bresenham's line on tile coordinates; each intermediate tile gets a
// bright 3×3 dot at its centre. A brief delay makes it visible, then tiles
// are restored from the map / drop / enemy data.
// After a flash effect has redrawn beam-path tiles from the static map, any
// entity that was mid-interpolation (drawn at a non-tile pixel position) is
// half-erased. This helper repaints every active entity at its current
// interpolated position and re-latches lastDrawn so drawSmoothEntitiesUpdate
// on the next frame knows where to start erasing from.
static void restoreInterpolatedEntities(Player* player) {
    int scale = combatScale();
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        int epx = PLAY_AREA_X + enemies[i].pixelX - s_cameraX;
        int epy = PLAY_AREA_Y + enemies[i].pixelY;
        drawEnemySprite(epx, epy, enemies[i].type, scale);
        enemies[i].lastDrawnPixelX = epx;
        enemies[i].lastDrawnPixelY = epy;
    }
    if (player) {
        int ppx = PLAY_AREA_X + player->pixelX - s_cameraX;
        int ppy = PLAY_AREA_Y + player->pixelY;
        drawPlayerAndChargeBar(player, ppx, ppy, scale);
        player->lastDrawnPixelX = ppx;
        player->lastDrawnPixelY = ppy;
    }
}

void drawBulletFlash(uint8_t mapData[MAP_ROWS][MAP_COLS],
                     int playerX, int playerY,
                     int targetX, int targetY,
                     int weaponType,
                     Player* player) {
    uint16_t col = COLOR_YELLOW;
    if (weaponType == WEAPON_MG) col = COLOR_ORANGE;
    else if (weaponType == WEAPON_LASER) col = COLOR_CYAN;

    // Collect tile path with Bresenham (max 16 steps)
    int pathX[16], pathY[16];
    int pathLen = 0;

    int x0 = playerX, y0 = playerY;
    int x1 = targetX, y1 = targetY;
    int dx =  abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (pathLen < 16) {
        pathX[pathLen] = x0;
        pathY[pathLen] = y0;
        pathLen++;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }

    // Draw: skip player tile (index 0), draw dots on intermediate + target tiles
    for (int i = 1; i < pathLen; i++) {
        int cx = PLAY_AREA_X + pathX[i] * TILE_SIZE + 7 - s_cameraX;  // tile centre pixel
        int cy = PLAY_AREA_Y + pathY[i] * TILE_SIZE + 7;

        // Skip drawing if outside view
        if (cx < PLAY_AREA_X || cx >= PLAY_AREA_X + PLAY_AREA_W) continue;

        int dotW = (i == pathLen - 1) ? 5 : 3;            // bigger dot on target
        lcd.fillRect(cx - dotW / 2, cy - dotW / 2, dotW, dotW, col);
    }

    delay(70);  // hold flash visible

    // Erase: redraw each affected tile from map / drops only. Entities are
    // restored afterwards at their interpolated pixel positions.
    for (int i = 1; i < pathLen; i++) {
        int tx = pathX[i], ty = pathY[i];
        drawTile(tx, ty, mapData[ty][tx]);
        if (dropExistsAt(tx, ty)) {
            drawDropIcon(tx, ty, dropTypeAt(tx, ty));
        }
    }
    restoreInterpolatedEntities(player);
}

// Red laser beam fired by a laser alien from (startX,startY) to (endX,endY).
// Straight line (axis-aligned). Flashes briefly then erases.
// player pointer allows the player sprite to be redrawn if the beam crosses their tile.
void drawLaserFlash(uint8_t mapData[MAP_ROWS][MAP_COLS],
                    int startX, int startY,
                    int endX,   int endY,
                    Player* player) {
    // Collect axis-aligned tiles between source and endpoint
    int pathX[16], pathY[16], pathLen = 0;
    int sx = (endX > startX) ? 1 : (endX < startX) ? -1 : 0;
    int sy = (endY > startY) ? 1 : (endY < startY) ? -1 : 0;
    int cx = startX, cy = startY;
    while (pathLen < 16) {
        pathX[pathLen] = cx; pathY[pathLen] = cy; pathLen++;
        if (cx == endX && cy == endY) break;
        cx += sx; cy += sy;
    }
    // Draw cyan-core red laser dots (skip tile 0 = enemy tile)
    for (int i = 1; i < pathLen; i++) {
        int tx = pathX[i], ty = pathY[i];
        if (tx < 0 || tx >= MAP_COLS || ty < 0 || ty >= MAP_ROWS) continue;
        int px = PLAY_AREA_X + tx * TILE_SIZE + 7 - s_cameraX;
        int py = PLAY_AREA_Y + ty * TILE_SIZE + 7;

        // Skip drawing if outside view
        if (px < PLAY_AREA_X || px >= PLAY_AREA_X + PLAY_AREA_W) continue;

        int dw = (i == pathLen - 1) ? 5 : 3;
        lcd.fillRect(px - dw/2, py - dw/2, dw, dw, 0xF800); // red beam
        lcd.fillRect(px,        py,         1,  1,  0x07FF); // cyan core
    }
    delay(60);
    // Erase �?guard every tile against out-of-bounds (status bar protection).
    // Only restore background here; entities are repainted below at their
    // interpolated positions so a mid-step player is not left as a ghost.
    for (int i = 1; i < pathLen; i++) {
        int tx = pathX[i], ty = pathY[i];
        if (tx < 0 || tx >= MAP_COLS || ty < 0 || ty >= MAP_ROWS) continue;
        drawTile(tx, ty, mapData[ty][tx]);
        if (dropExistsAt(tx, ty)) drawDropIcon(tx, ty, dropTypeAt(tx, ty));
    }
    restoreInterpolatedEntities(player);
}

extern int g_recoilTiltAmount;
extern int g_recoilDir;

void drawLaserCannonFlash(uint8_t mapData[MAP_ROWS][MAP_COLS],
                          int startTileX, int startTileY, int dir, bool isPlatformer,
                          Player* player, int recoilX, int recoilY) {
    int originalCameraX = s_cameraX;

    float pX = player->pixelX;
    float pY = player->pixelY;
    int numFrames = 8;
    float stepX = recoilX / (float)numFrames;
    float stepY = recoilY / (float)numFrames;

    g_recoilDir = dir;
    int scale = combatScale();

    int beamRx = PLAY_AREA_X, beamRy = PLAY_AREA_Y, beamRw = 0, beamRh = 0;

    for (int frame = 0; frame < numFrames; frame++) {
        int oldPx = (int)pX;
        int oldPy = (int)pY;
        pX += stepX;
        pY += stepY;

        // Bounds checking
        if (pX < 0) pX = 0;
        if (pX > (MAP_COLS * TILE_SIZE) - 16) pX = (MAP_COLS * TILE_SIZE) - 16;
        if (pY < 0) pY = 0;
        if (pY > (MAP_ROWS * TILE_SIZE) - 16) pY = (MAP_ROWS * TILE_SIZE) - 16;

        player->pixelX = (int)pX;
        player->pixelY = (int)pY;
        player->tileX = (player->pixelX + 8) / TILE_SIZE;
        player->tileY = (player->pixelY + 8) / TILE_SIZE;

        int currentPx = PLAY_AREA_X + player->pixelX - originalCameraX;
        int currentPy = PLAY_AREA_Y + player->pixelY;

        int rx = currentPx, ry = currentPy, rw = 16, rh = 16;
        if (dir == CHAR_DIR_LEFT) {
            rx = PLAY_AREA_X; rw = currentPx - PLAY_AREA_X + 8;
            ry = currentPy - 8; rh = 32;
        } else if (dir == CHAR_DIR_RIGHT) {
            rx = currentPx + 8; rw = PLAY_AREA_X + PLAY_AREA_W - rx;
            ry = currentPy - 8; rh = 32;
        } else if (dir == CHAR_DIR_UP) {
            rx = currentPx - 8; rw = 32;
            ry = PLAY_AREA_Y; rh = currentPy - PLAY_AREA_Y + 8;
        } else if (dir == CHAR_DIR_DOWN) {
            rx = currentPx - 8; rw = 32;
            ry = currentPy + 8; rh = PLAY_AREA_Y + PLAY_AREA_H - ry;
        }

        // clip beam to play area
        if (rx < PLAY_AREA_X) { rw -= (PLAY_AREA_X - rx); rx = PLAY_AREA_X; }
        if (ry < PLAY_AREA_Y) { rh -= (PLAY_AREA_Y - ry); ry = PLAY_AREA_Y; }
        if (rx + rw > PLAY_AREA_X + PLAY_AREA_W) rw = PLAY_AREA_X + PLAY_AREA_W - rx;
        if (ry + rh > PLAY_AREA_Y + PLAY_AREA_H) rh = PLAY_AREA_Y + PLAY_AREA_H - ry;

        // Erase old player area
        int eraseX = PLAY_AREA_X + oldPx - originalCameraX - 16;
        int eraseY = PLAY_AREA_Y + oldPy - 16;
        int eraseW = 48; // player size is max 24x24, dust also drawn
        int eraseH = 48;
        redrawTileRect(mapData, eraseX, eraseY, eraseW, eraseH);

        // Draw beam
        if (rw > 0 && rh > 0) {
            if (frame % 2 == 0) {
                lcd.fillRect(rx, ry, rw, rh, COLOR_CYAN);
                if (dir == CHAR_DIR_LEFT || dir == CHAR_DIR_RIGHT) {
                    lcd.fillRect(rx, ry + 8, rw, 16, COLOR_WHITE);
                } else {
                    lcd.fillRect(rx + 8, ry, 16, rh, COLOR_WHITE);
                }
            } else {
                if (dir == CHAR_DIR_LEFT || dir == CHAR_DIR_RIGHT) {
                    lcd.fillRect(rx, ry + 4, rw, 24, 0x03EF);
                    lcd.fillRect(rx, ry + 12, rw, 8, COLOR_CYAN);
                } else {
                    lcd.fillRect(rx + 4, ry, 24, rh, 0x03EF);
                    lcd.fillRect(rx + 12, ry, 8, rh, COLOR_CYAN);
                }
            }
            beamRx = rx; beamRy = ry; beamRw = rw; beamRh = rh;
        }

        // Draw dust particles at the feet depending on recoil direction
        int feetX = currentPx + 8;
        int feetY = currentPy + 20;
        if (frame > 2 && (dir == CHAR_DIR_LEFT || dir == CHAR_DIR_RIGHT)) {
            for (int d = 0; d < 3; d++) {
                int dustX = feetX + (rand() % 16 - 8);
                int dustY = feetY + (rand() % 8 - 4);
                if (dir == CHAR_DIR_LEFT) dustX -= (rand() % 12); // sliding right, dust on left
                else if (dir == CHAR_DIR_RIGHT) dustX += (rand() % 12); // sliding left, dust on right
                uint16_t dustColor = (rand() % 2 == 0) ? 0xCE59 : 0x9CD3; // grey/brown
                if (dustX >= PLAY_AREA_X && dustX < PLAY_AREA_X + PLAY_AREA_W &&
                    dustY >= PLAY_AREA_Y && dustY < PLAY_AREA_Y + PLAY_AREA_H) {
                    lcd.fillRect(dustX, dustY, 2, 2, dustColor);
                }
            }
        }

        // Draw Player with tilt
        g_recoilTiltAmount = (frame < 3) ? (frame * 2) : (numFrames - frame);
        drawPlayerAndChargeBar(player, currentPx, currentPy, scale);

        delay(25);
    }

    g_recoilTiltAmount = 0;

    // Erase beam and the last player state bounding box
    if (beamRw > 0 && beamRh > 0) {
        redrawTileRect(mapData, beamRx, beamRy, beamRw, beamRh);
    }
    int finalPx = PLAY_AREA_X + player->pixelX - originalCameraX;
    int finalPy = PLAY_AREA_Y + player->pixelY;
    redrawTileRect(mapData, finalPx - 16, finalPy - 16, 48, 48);

    drawAllDrops();
    drawPortal();

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        int epx = PLAY_AREA_X + enemies[i].pixelX - s_cameraX;
        int epy = PLAY_AREA_Y + enemies[i].pixelY;
        drawEnemySprite(epx, epy, enemies[i].type, scale);
        enemies[i].lastDrawnPixelX = epx;
        enemies[i].lastDrawnPixelY = epy;
    }
    if (player) {
        int ppx = PLAY_AREA_X + player->pixelX - s_cameraX;
        int ppy = PLAY_AREA_Y + player->pixelY;
        drawPlayerAndChargeBar(player, ppx, ppy, scale);
        player->lastDrawnPixelX = ppx;
        player->lastDrawnPixelY = ppy;
    }
}

void drawBootScreen() {
    lcd.fillScreen(COLOR_BLACK);
}

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

static void uiDrawLine(int x0, int y0, int x1, int y1, uint16_t color) {
    if (y0 == y1) {
        int lx = (x0 < x1) ? x0 : x1;
        lcd.drawFastHLine(lx, y0, abs(x1 - x0) + 1, color);
        return;
    }
    if (x0 == x1) {
        int ly = (y0 < y1) ? y0 : y1;
        lcd.drawFastVLine(x0, ly, abs(y1 - y0) + 1, color);
        return;
    }

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    while (true) {
        lcd.drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

static int uiSmoothLerp(int start, int end, uint32_t elapsed, uint32_t duration) {
    if (duration == 0) return end;
    if (elapsed >= duration) return end;

    uint32_t p = (elapsed * 1024UL) / duration;                  // 0..1024
    uint32_t smooth = (p * p * (3072UL - 2UL * p)) / 1048576UL;  // smoothstep
    long delta = (long)end - (long)start;
    return start + (int)((delta * (long)smooth) / 1024L);
}

static void drawAltarArrivalShip(int cx, int cy, bool flashOn) {
    uint16_t ringOuter = flashOn ? COLOR_GOLD : 0x8400;
    uint16_t ringInner = flashOn ? 0xA81F : 0x680D;
    uint16_t dome = flashOn ? COLOR_CYAN : 0x041F;
    uint16_t engine = flashOn ? COLOR_ORANGE : 0xC300;

    drawMenuCircle(cx, cy, 18, ringOuter);
    drawMenuCircle(cx, cy, 20, ringInner);
    lcd.fillRect(cx - 24, cy - 5, 48, 10, COLOR_DARKGRAY);
    lcd.fillRect(cx - 16, cy - 10, 32, 8, COLOR_GRAY);
    lcd.fillRect(cx - 8, cy - 14, 16, 6, COLOR_LIGHTGRAY);
    lcd.fillRect(cx - 7, cy - 11, 14, 7, dome);
    drawMenuCircle(cx, cy - 7, 7, COLOR_WHITE);

    lcd.fillRect(cx - 18, cy + 4, 4, 3, engine);
    lcd.fillRect(cx - 6,  cy + 4, 4, 3, engine);
    lcd.fillRect(cx + 4,  cy + 4, 4, 3, engine);
    lcd.fillRect(cx + 14, cy + 4, 4, 3, engine);

    drawMenuCircle(cx, cy - 28, 8, COLOR_MAGENTA);
    lcd.fillRect(cx - 2, cy - 31, 4, 6, COLOR_MAGENTA);
    lcd.fillRect(cx - 6, cy - 29, 12, 2, COLOR_MAGENTA);
    lcd.fillRect(cx - 1, cy - 28, 2, 2, COLOR_YELLOW);
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
    drawMenuSpriteLayer(px, py, sBeastBody,  COL_BEAST_SKIN);
    drawMenuSpriteLayer(px, py, sBeastEyes,  COL_BEAST_EYES);
    drawMenuSpriteLayer(px, py, sBeastClaws, COL_BEAST_CLAW);
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
    lcd.fillRect(x + 4, y + 66, 80, 20, 0x6B85);
    for (int i = 0; i < 7; i++) {
        int cx = x + 9 + i * 10;
        lcd.drawFastVLine(cx, y + 68 + (i & 1), 16, 0xFFE0);
        lcd.drawFastVLine(cx + 2, y + 70 + ((i + 1) & 1), 12, 0xFFE0);
    }
    lcd.fillRect(x + 56, y + 50, 18, 12, 0x3C8F);
    lcd.drawRect(x + 56, y + 50, 18, 12, 0x7D9F);
    lcd.drawPixel(x + 61, y + 54, 0xFFFF);
    lcd.drawPixel(x + 67, y + 58, 0xFFFF);

    lcd.fillRect(x + 8, y + 50, 12, 8, 0xFFFF);
    lcd.fillRect(x + 9, y + 54, 10, 6, 0xFFFF);
    lcd.drawPixel(x + 11, y + 55, 0x0000);
    lcd.drawPixel(x + 16, y + 55, 0x0000);
    lcd.fillRect(x + 12, y + 58, 2, 8, 0x9CF3);
    lcd.fillRect(x + 17, y + 58, 2, 8, 0x9CF3);
    lcd.fillRect(x + 14, y + 52, 4, 2, 0x212C);

    lcd.fillRect(x + 24, y + 54, 12, 8, 0xFFFF);
    lcd.fillRect(x + 25, y + 58, 10, 6, 0xFFFF);
    lcd.drawPixel(x + 27, y + 59, 0x0000);
    lcd.drawPixel(x + 32, y + 59, 0x0000);
    lcd.fillRect(x + 28, y + 62, 2, 8, 0x9CF3);
    lcd.fillRect(x + 33, y + 62, 2, 8, 0x9CF3);
    lcd.fillRect(x + 30, y + 56, 4, 2, 0x212C);

    characterDrawFarmer(x + 38, y + 48);

    lcd.fillRect(x + 17, y + 94, 54, 14, 0x18C6);
    lcd.drawRect(x + 17, y + 94, 54, 14, COLOR_YELLOW);
    lcd.drawText(x + 24, y + 98, "Farming", COLOR_WHITE, 0x18C6, 1);
}

void drawMainMenu() {
    consoleDrawMainMenu();
}

void drawExploreScreen(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player) {
    lcd.fillScreen(COLOR_BLACK);

    drawStatusBar(player);
    drawBottomBar(player->weapon.type);

    // Tiles cover every pixel of the play area �?no separate fill or border needed

    int startCol = s_cameraX / TILE_SIZE;
    int endCol = startCol + VISIBLE_COLS + 1;
    if (endCol > MAP_COLS) endCol = MAP_COLS;

    // Use a row buffer to render tiles row-by-row instantly, avoiding the visible "line-by-line" tearing
    static uint16_t rowBuf[PLAY_AREA_W * TILE_SIZE];

    for (int y = 0; y < MAP_ROWS; y++) {
        int py = PLAY_AREA_Y + y * TILE_SIZE;
        if (py + TILE_SIZE <= PLAY_AREA_Y || py >= PLAY_AREA_Y + PLAY_AREA_H) continue;

        int rh = TILE_SIZE;
        if (py < PLAY_AREA_Y) { rh -= (PLAY_AREA_Y - py); py = PLAY_AREA_Y; }
        if (py + rh > PLAY_AREA_Y + PLAY_AREA_H) { rh = PLAY_AREA_Y + PLAY_AREA_H - py; }

        lcd.setRenderTarget(rowBuf, PLAY_AREA_X, py, PLAY_AREA_W, rh);

        for (int x = startCol; x < endCol; x++) {
            drawTile(x, y, mapData[y][x]);
        }

        lcd.setRenderTarget(nullptr, 0, 0, 0, 0);
        lcd.pushImage(PLAY_AREA_X, py, PLAY_AREA_W, rh, rowBuf);
    }

    drawAllDrops();   // draw loot icons on top of tiles
    drawPortal();     // restore portal sprite if active
    drawEnemies();    // draw enemy sprites
    drawPlayer(player);
}

// 增量更新：只重绘玩家旧位置和新位置的格子，无需全屏刷新
void drawExploreUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player, int oldX, int oldY) {
    // Draw player at NEW position FIRST �?player stays visible the whole time, no flicker
    drawPlayer(player);
    // THEN erase old position (restores map tile, portal, drop)
    drawTile(oldX, oldY, mapData[oldY][oldX]);
    if (portal.active && portal.tileX == oldX && portal.tileY == oldY) {
        drawPortalTile(oldX, oldY);
    }
    if (dropExistsAt(oldX, oldY)) {
        drawDropIcon(oldX, oldY, dropTypeAt(oldX, oldY));
    }
}

// 只重绘游戏区格子和玩家，不清全屏（用于关�?overlay 后恢复地图）
void drawPlayArea(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player) {
    int startCol = s_cameraX / TILE_SIZE;
    int endCol = startCol + VISIBLE_COLS + 1;
    if (endCol > MAP_COLS) endCol = MAP_COLS;

    static uint16_t rowBuf[PLAY_AREA_W * TILE_SIZE];

    for (int y = 0; y < MAP_ROWS; y++) {
        int py = PLAY_AREA_Y + y * TILE_SIZE;
        if (py + TILE_SIZE <= PLAY_AREA_Y || py >= PLAY_AREA_Y + PLAY_AREA_H) continue;

        int rh = TILE_SIZE;
        if (py < PLAY_AREA_Y) { rh -= (PLAY_AREA_Y - py); py = PLAY_AREA_Y; }
        if (py + rh > PLAY_AREA_Y + PLAY_AREA_H) { rh = PLAY_AREA_Y + PLAY_AREA_H - py; }

        lcd.setRenderTarget(rowBuf, PLAY_AREA_X, py, PLAY_AREA_W, rh);

        for (int x = startCol; x < endCol; x++) {
            drawTile(x, y, mapData[y][x]);
        }

        lcd.setRenderTarget(nullptr, 0, 0, 0, 0);
        lcd.pushImage(PLAY_AREA_X, py, PLAY_AREA_W, rh, rowBuf);
    }
    drawAllDrops();
    drawPortal();     // restore portal sprite if active
    drawEnemies();
    drawPlayer(player);
}

// -------------------------------------------------------
// Inventory overlay �?drawn ON TOP of the map, no clear
// Panel: x=14, y=28, w=212, h=170 (fits inside play area)
// -------------------------------------------------------

// Panel constants (local to ui.cpp)
#define INV_PX   14
#define INV_PY   28
#define INV_PW  212
#define INV_PH  170

// Draw one item row inside the overlay panel
static void drawItemRow(int y, uint16_t dotColor, const char* label, int qty) {
    // color dot
    lcd.fillRect(INV_PX + 8, y + 3, 8, 8, dotColor);
    lcd.drawRect(INV_PX + 8, y + 3, 8, 8, COLOR_WHITE);
    // item name
    lcd.drawText(INV_PX + 22, y, label, COLOR_WHITE, COLOR_BG_PANEL, 1);
    // quantity, right-aligned inside panel
    char buf[6];
    sprintf(buf, "x%d", qty);
    lcd.drawText(INV_PX + INV_PW - 30, y, buf, COLOR_YELLOW, COLOR_BG_PANEL, 1);
}

// Update only the food quantity text inside the already-visible inventory overlay.
// Food row is at rowStart+26 = (INV_PY+26)+26. Quantity sits at INV_PX+INV_PW-30.
void drawInventoryFoodCount(int qty) {
    int x = INV_PX + INV_PW - 30;
    int y = INV_PY + 26 + 26;   // same formula as drawItemRow call for Food
    lcd.fillRect(x, y, 30, 9, COLOR_BG_PANEL);  // erase old number
    char buf[6];
    sprintf(buf, "x%d", qty);
    lcd.drawText(x, y, buf, COLOR_YELLOW, COLOR_BG_PANEL, 1);
}

void drawInventoryArmorCount(int qty) {
    int x = INV_PX + INV_PW - 30;
    int y = INV_PY + 26 + 78;   // same formula as drawItemRow call for Armor
    lcd.fillRect(x, y, 30, 9, COLOR_BG_PANEL);  // erase old number
    char buf[6];
    sprintf(buf, "x%d", qty);
    lcd.drawText(x, y, buf, COLOR_YELLOW, COLOR_BG_PANEL, 1);
}

void drawInventoryOverlay(Player* player) {
    // Dark panel background with border
    lcd.fillRect(INV_PX, INV_PY, INV_PW, INV_PH, COLOR_BG_PANEL);
    lcd.drawRect(INV_PX, INV_PY, INV_PW, INV_PH, COLOR_LIGHTGRAY);

    // Title bar
    lcd.fillRect(INV_PX + 1, INV_PY + 1, INV_PW - 2, 18, COLOR_DARKGRAY);
    lcd.drawText(INV_PX + 66, INV_PY + 5, "BACKPACK", COLOR_YELLOW, COLOR_DARKGRAY, 1);
    lcd.drawFastHLine(INV_PX + 1, INV_PY + 19, INV_PW - 2, COLOR_LIGHTGRAY);

    // 5 item rows (scale 1: char height ~8px, row pitch 26px)
    int rowStart = INV_PY + 26;
    drawItemRow(rowStart +  0, COLOR_GREEN,     "Seeds",        player->inv.seeds);
    drawItemRow(rowStart + 26, COLOR_ORANGE,    "Food",         player->inv.food);
    drawItemRow(rowStart + 52, COLOR_LIGHTGRAY, "Metal",        player->inv.metal);
    drawItemRow(rowStart + 78, COLOR_CYAN,      "Armor",        player->inv.armor);
    drawItemRow(rowStart +104, COLOR_MAGENTA,   "Alien Relics", player->inv.alienRelics);

    // Hint bar at bottom of panel �?left: eat, right: close
    lcd.drawFastHLine(INV_PX + 1, INV_PY + INV_PH - 14, INV_PW - 2, COLOR_DARKGRAY);
    lcd.drawText(INV_PX + 8,  INV_PY + INV_PH - 11, "B:FOOD", COLOR_ORANGE, COLOR_BG_PANEL, 1);
    lcd.drawText(INV_PX + 64, INV_PY + INV_PH - 11, "C:ARMOR", COLOR_CYAN, COLOR_BG_PANEL, 1);
    lcd.drawText(INV_PX + 142, INV_PY + INV_PH - 11, "JOY:CLOSE",  COLOR_GRAY, COLOR_BG_PANEL, 1);
}

// -------------------------------------------------------
// Portal �?black hole sprite + bottom-bar slot 3 prompt
// -------------------------------------------------------

// Black-hole layered rings on a single 16×16 tile.
// Colors (RGB565): dark indigo �?deep purple �?bright purple �?void core
// Accretion disk: 4 orange hotspots at N/S/E/W of the inner ring
// Event-horizon shimmer: 4 tiny cyan pixels on diagonal corners
void drawPortalTile(int tileX, int tileY) {
    int px = PLAY_AREA_X + tileX * TILE_SIZE - s_cameraX;
    int py = PLAY_AREA_Y + tileY * TILE_SIZE;

    if (px <= PLAY_AREA_X - TILE_SIZE || px >= PLAY_AREA_X + PLAY_AREA_W) return;

    lcd.fillRect(px,   py,   16, 16, 0x0803);  // outer dark-indigo glow
    lcd.fillRect(px+2, py+2, 12, 12, 0x2008);  // deep purple ring
    lcd.fillRect(px+4, py+4,  8,  8, 0x400C);  // bright purple inner ring
    lcd.fillRect(px+6, py+6,  4,  4, 0x1005);  // near-black void
    lcd.fillRect(px+7, py+7,  2,  2, 0x0000);  // pure-black singularity

    // Accretion disk �?4 orange hotspots (N / S / W / E)
    lcd.fillRect(px+7,  py+4,  2, 1, COLOR_ORANGE);
    lcd.fillRect(px+7,  py+11, 2, 1, COLOR_ORANGE);
    lcd.fillRect(px+4,  py+7,  1, 2, COLOR_ORANGE);
    lcd.fillRect(px+11, py+7,  1, 2, COLOR_ORANGE);

    // Event-horizon shimmer �?4 tiny cyan sparks on diagonal edge of inner ring
    lcd.fillRect(px+5, py+5, 1, 1, COLOR_CYAN);
    lcd.fillRect(px+9, py+5, 1, 1, COLOR_CYAN);
    lcd.fillRect(px+5, py+9, 1, 1, COLOR_CYAN);
    lcd.fillRect(px+9, py+9, 1, 1, COLOR_CYAN);
}

// Draw portal only if active �?safe to call after any incremental tile update
void drawPortal() {
    if (portal.active) {
        drawPortalTile(portal.tileX, portal.tileY);
    }
}

// Slot 3 of the bottom bar (x=164, y=204, w=72, h=28).
// Only the interior is redrawn �?border stays, no other slots touched.
void drawPortalSlot(bool nearPortal) {
    lcd.fillRect(165, 205, 70, 26, COLOR_BG_PANEL);
    if (nearPortal) {
        lcd.drawText(191, 210, "[A]",   COLOR_CYAN,      COLOR_BG_PANEL, 1);
        lcd.drawText(185, 221, "ENTER", COLOR_YELLOW,    COLOR_BG_PANEL, 1);
    } else {
        drawAreaName(172, 210);
    }
}

void drawAltarDialogue() {
    const int boxX = 16;
    const int boxY = 136;
    const int boxW = 208;
    const int boxH = 52;
    const uint16_t bg = 0x1006;
    const uint16_t border = 0xA81F;

    lcd.fillRect(boxX, boxY, boxW, boxH, bg);
    lcd.drawRect(boxX, boxY, boxW, boxH, border);
    lcd.drawRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, 0x39CE);

    lcd.drawText(boxX + 8, boxY + 6,  "Dawn-Warden!", COLOR_YELLOW, bg, 1);
    lcd.drawText(boxX + 8, boxY + 17, "The altar grants thee", COLOR_WHITE, bg, 1);
    lcd.drawText(boxX + 8, boxY + 27, "unbounded might.", COLOR_WHITE, bg, 1);
    lcd.drawText(boxX + 8, boxY + 37, "Beware the curse of power:", COLOR_MAGENTA, bg, 1);
    lcd.drawText(boxX + 8, boxY + 47, "We unmake them, not be unmade.", COLOR_LIGHTGRAY, bg, 1);
}

void drawSkullDialogue() {
    const int boxX = 16;
    const int boxY = 92;
    const int boxW = 208;
    const int boxH = 96;
    const uint16_t bg = 0x0804;
    const uint16_t border = 0xF81F;

    lcd.fillRect(boxX, boxY, boxW, boxH, bg);
    lcd.drawRect(boxX, boxY, boxW, boxH, border);
    lcd.drawRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, 0x39CE);

    lcd.drawText(boxX + 8, boxY + 10, "Power... in my bones...", COLOR_YELLOW, bg, 1);
    lcd.drawText(boxX + 8, boxY + 26, "Altar... my master...", COLOR_WHITE, bg, 1);
    lcd.drawText(boxX + 8, boxY + 42, "Foes? Gods...", COLOR_MAGENTA, bg, 1);
    lcd.drawText(boxX + 8, boxY + 58, "I am their blade.", COLOR_LIGHTGRAY, bg, 1);
    lcd.drawText(boxX + 8, boxY + 74, "Light is dead.", COLOR_RED, bg, 1);
}

void drawSkullMiracleDialogue() {
    const int boxX = 18;
    const int boxY = 160;
    const int boxW = 204;
    const int boxH = 24;
    const uint16_t bg = 0x0804;
    const uint16_t border = 0xA81F;

    lcd.fillRect(boxX, boxY, boxW, boxH, bg);
    lcd.drawRect(boxX, boxY, boxW, boxH, border);
    lcd.drawRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, 0x39CE);
    lcd.drawText(boxX + 8, boxY + 8, "The miracle returns, my master!", COLOR_YELLOW, bg, 1);
}

void drawSupplyWarriorDialogue() {
    const int boxX = 16;
    const int boxY = 136;
    const int boxW = 208;
    const int boxH = 52;
    const uint16_t bg = 0x1006;
    const uint16_t border = 0xF800;

    lcd.fillRect(boxX, boxY, boxW, boxH, bg);
    lcd.drawRect(boxX, boxY, boxW, boxH, border);
    lcd.drawRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, 0x39CE);
    lcd.drawText(boxX + 8, boxY + 6,  "They've grown bolder", COLOR_YELLOW, bg, 1);
    lcd.drawText(boxX + 8, boxY + 17, "by the day.", COLOR_WHITE, bg, 1);
    lcd.drawText(boxX + 8, boxY + 31, "We've already lost eight", COLOR_WHITE, bg, 1);
    lcd.drawText(boxX + 8, boxY + 42, "forward sentinels.", COLOR_LIGHTGRAY, bg, 1);
}

void drawSupplyCivilianDialogue() {
    const int boxX = 16;
    const int boxY = 148;
    const int boxW = 208;
    const int boxH = 40;
    const uint16_t bg = 0x18A6;
    const uint16_t border = 0xFFE0;

    lcd.fillRect(boxX, boxY, boxW, boxH, bg);
    lcd.drawRect(boxX, boxY, boxW, boxH, border);
    lcd.drawRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, 0x39CE);
    lcd.drawText(boxX + 8, boxY + 8,  "Are you a forward sentinel?", COLOR_WHITE, bg, 1);
    lcd.drawText(boxX + 8, boxY + 22, "Thank you for protecting us!", COLOR_YELLOW, bg, 1);
}

void drawSupplyWizardDialogue() {
    const int boxX = 16;
    const int boxY = 136;
    const int boxW = 208;
    const int boxH = 52;
    const uint16_t bg = 0x0804;
    const uint16_t border = 0xA81F;

    lcd.fillRect(boxX, boxY, boxW, boxH, bg);
    lcd.drawRect(boxX, boxY, boxW, boxH, border);
    lcd.drawRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, 0x39CE);
    lcd.drawText(boxX + 8, boxY + 6,  "Things grow ever more", COLOR_MAGENTA, bg, 1);
    lcd.drawText(boxX + 8, boxY + 20, "costly here...", COLOR_WHITE, bg, 1);
    lcd.drawText(boxX + 8, boxY + 34, "this is blasphemy", COLOR_WHITE, bg, 1);
    lcd.drawText(boxX + 8, boxY + 45, "against the gods.", COLOR_LIGHTGRAY, bg, 1);
}

void clearAltarDialogueRegion(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player) {
    int startCol = s_cameraX / TILE_SIZE;
    int endCol = startCol + VISIBLE_COLS + 1;
    if (endCol > MAP_COLS) endCol = MAP_COLS;

    static uint16_t rowBuf[PLAY_AREA_W * TILE_SIZE];

    for (int y = 4; y < MAP_ROWS; y++) {
        int py = PLAY_AREA_Y + y * TILE_SIZE;
        if (py + TILE_SIZE <= PLAY_AREA_Y || py >= PLAY_AREA_Y + PLAY_AREA_H) continue;

        int rh = TILE_SIZE;
        if (py < PLAY_AREA_Y) { rh -= (PLAY_AREA_Y - py); py = PLAY_AREA_Y; }
        if (py + rh > PLAY_AREA_Y + PLAY_AREA_H) { rh = PLAY_AREA_Y + PLAY_AREA_H - py; }

        lcd.setRenderTarget(rowBuf, PLAY_AREA_X, py, PLAY_AREA_W, rh);

        for (int x = startCol; x < endCol; x++) {
            drawTile(x, y, mapData[y][x]);
        }

        lcd.setRenderTarget(nullptr, 0, 0, 0, 0);
        lcd.pushImage(PLAY_AREA_X, py, PLAY_AREA_W, rh, rowBuf);
    }
    drawAllDrops();
    drawPortal();
    drawEnemies();
    drawPlayer(player);
}

void clearSupplyDialogueRegion(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player) {
    int startCol = s_cameraX / TILE_SIZE;
    int endCol = startCol + VISIBLE_COLS + 1;
    if (endCol > MAP_COLS) endCol = MAP_COLS;

    static uint16_t rowBuf[PLAY_AREA_W * TILE_SIZE];

    for (int y = 4; y < MAP_ROWS; y++) {
        int py = PLAY_AREA_Y + y * TILE_SIZE;
        if (py + TILE_SIZE <= PLAY_AREA_Y || py >= PLAY_AREA_Y + PLAY_AREA_H) continue;

        int rh = TILE_SIZE;
        if (py < PLAY_AREA_Y) { rh -= (PLAY_AREA_Y - py); py = PLAY_AREA_Y; }
        if (py + rh > PLAY_AREA_Y + PLAY_AREA_H) { rh = PLAY_AREA_Y + PLAY_AREA_H - py; }

        lcd.setRenderTarget(rowBuf, PLAY_AREA_X, py, PLAY_AREA_W, rh);

        for (int x = startCol; x < endCol; x++) {
            drawTile(x, y, mapData[y][x]);
        }

        lcd.setRenderTarget(nullptr, 0, 0, 0, 0);
        lcd.pushImage(PLAY_AREA_X, py, PLAY_AREA_W, rh, rowBuf);
    }
    drawAllDrops();
    drawPortal();
    drawEnemies();
    drawPlayer(player);
}

void drawSupplyTradePrompt() {
    const int boxX = 114;
    const int boxY = 174;
    const int boxW = 118;
    const int boxH = 22;
    const uint16_t bg = 0x1065;

    lcd.fillRect(boxX, boxY, boxW, boxH, bg);
    lcd.drawRect(boxX, boxY, boxW, boxH, 0xC618);
    lcd.drawText(boxX + 8, boxY + 4, "PRESS B TO", COLOR_YELLOW, bg, 1);
    lcd.drawText(boxX + 8, boxY + 13, "TRADE", COLOR_WHITE, bg, 1);
}

static void drawSupplyTradeRow(Player* player, int index, int selection) {
    const int boxX = 14;
    const int boxY = 30;
    const uint16_t rowBg = 0x18A6;
    const uint16_t selBg = 0x39CE;
    const char* haveA[4] = {"Seeds", "Metal", "Relics", "Food"};
    const char* haveB[4] = {"x3",    "x4",    "x2",     "x6"};
    const char* getA[4]  = {"Food x1", "Food x2", "Metal x3", "Blood Pack"};
    int qtyA[4] = {player->inv.seeds, player->inv.metal, player->inv.alienRelics, player->inv.food};
    uint16_t dot[4] = {COLOR_GREEN, COLOR_LIGHTGRAY, COLOR_MAGENTA, COLOR_ORANGE};

    int rowY = boxY + 38 + index * 24;
    uint16_t fill = (index == selection) ? selBg : rowBg;
    lcd.fillRect(boxX + 8, rowY, 196, 20, fill);
    lcd.drawRect(boxX + 8, rowY, 196, 20, (index == selection) ? COLOR_YELLOW : 0x4A69);
    lcd.fillRect(boxX + 12, rowY + 6, 6, 6, dot[index]);
    lcd.drawRect(boxX + 12, rowY + 6, 6, 6, COLOR_WHITE);
    lcd.drawText(boxX + 24, rowY + 5, haveA[index], COLOR_WHITE, fill, 1);
    lcd.drawText(boxX + 72, rowY + 5, haveB[index], COLOR_YELLOW, fill, 1);
    lcd.drawText(boxX + 108, rowY + 5, "->", COLOR_WHITE, fill, 1);
    lcd.drawText(boxX + 132, rowY + 5, getA[index], COLOR_CYAN, fill, 1);

    char buf[6];
    sprintf(buf, "%d", qtyA[index]);
    lcd.fillRect(boxX + 182, rowY + 5, 18, 8, fill);
    lcd.drawText(boxX + 186, rowY + 5, buf, COLOR_WHITE, fill, 1);
}

void drawSupplyTradeOverlay(Player* player, int selection, const char* notice, bool noticeError) {
    const int boxX = 14;
    const int boxY = 30;
    const int boxW = 212;
    const int boxH = 166;
    const uint16_t bg = 0x0847;

    lcd.fillRect(boxX, boxY, boxW, boxH, bg);
    lcd.drawRect(boxX, boxY, boxW, boxH, COLOR_LIGHTGRAY);
    lcd.fillRect(boxX + 1, boxY + 1, boxW - 2, 18, COLOR_DARKGRAY);
    lcd.drawText(boxX + 66, boxY + 5, "TRADE POST", COLOR_YELLOW, COLOR_DARKGRAY, 1);
    lcd.drawFastHLine(boxX + 1, boxY + 19, boxW - 2, COLOR_LIGHTGRAY);

    lcd.drawText(boxX + 10, boxY + 26, "ITEMS", COLOR_CYAN, bg, 1);
    lcd.drawText(boxX + 138, boxY + 26, "DEAL", COLOR_CYAN, bg, 1);

    for (int i = 0; i < 4; i++) {
        drawSupplyTradeRow(player, i, selection);
    }

    lcd.fillRect(boxX + 8, boxY + 138, boxW - 16, 16, 0x1065);
    if (notice && notice[0]) {
        lcd.drawText(boxX + 12, boxY + 142, notice, noticeError ? COLOR_RED : COLOR_GREEN, 0x1065, 1);
    } else {
        lcd.drawText(boxX + 12, boxY + 142, "UP/DOWN: SELECT", COLOR_WHITE, 0x1065, 1);
    }

    lcd.drawFastHLine(boxX + 1, boxY + boxH - 14, boxW - 2, COLOR_DARKGRAY);
    lcd.drawText(boxX + 8, boxY + boxH - 11, "B:TRADE", COLOR_YELLOW, bg, 1);
    lcd.drawText(boxX + 78, boxY + boxH - 11, "A:CLOSE", COLOR_CYAN, bg, 1);
    lcd.drawText(boxX + 144, boxY + boxH - 11, "JOY:MOVE", COLOR_LIGHTGRAY, bg, 1);
}

void drawSupplyTradeSelection(Player* player, int oldSelection, int newSelection) {
    if (oldSelection >= 0 && oldSelection < 4) {
        drawSupplyTradeRow(player, oldSelection, newSelection);
    }
    if (newSelection >= 0 && newSelection < 4 && newSelection != oldSelection) {
        drawSupplyTradeRow(player, newSelection, newSelection);
    }
}

static void redrawPlayRect(uint8_t mapData[MAP_ROWS][MAP_COLS],
                           Player* player,
                           int x, int y, int w, int h) {
    int left   = x;
    int top    = y;
    int right  = x + w - 1;
    int bottom = y + h - 1;

    int playLeft   = PLAY_AREA_X;
    int playTop    = PLAY_AREA_Y;
    int playRight  = PLAY_AREA_X + MAP_COLS * TILE_SIZE - 1;
    int playBottom = PLAY_AREA_Y + MAP_ROWS * TILE_SIZE - 1;

    if (left < playLeft) left = playLeft;
    if (top < playTop) top = playTop;
    if (right > playRight) right = playRight;
    if (bottom > playBottom) bottom = playBottom;
    if (left > right || top > bottom) return;

    int tileX0 = (left - PLAY_AREA_X) / TILE_SIZE;
    int tileY0 = (top - PLAY_AREA_Y) / TILE_SIZE;
    int tileX1 = (right - PLAY_AREA_X) / TILE_SIZE;
    int tileY1 = (bottom - PLAY_AREA_Y) / TILE_SIZE;

    for (int ty = tileY0; ty <= tileY1; ty++) {
        for (int tx = tileX0; tx <= tileX1; tx++) {
            drawTile(tx, ty, mapData[ty][tx]);
            if (dropExistsAt(tx, ty)) {
                drawDropIcon(tx, ty, dropTypeAt(tx, ty));
            }
            if (portal.active && portal.tileX == tx && portal.tileY == ty) {
                drawPortalTile(tx, ty);
            }
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (enemies[e].active && enemies[e].tileX == tx && enemies[e].tileY == ty) {
                    int epx = PLAY_AREA_X + tx * TILE_SIZE;
                    int epy = PLAY_AREA_Y + ty * TILE_SIZE;
                    drawEnemySprite(epx, epy, enemies[e].type, combatScale());
                }
            }
            if (player && player->tileX == tx && player->tileY == ty) {
                drawPlayer(player);
            }
        }
    }
}

void drawOfferRelicsPrompt(bool hasRelics) {
    const int boxX = 118;
    const int boxY = 174;
    const int boxW = 114;
    const int boxH = 22;
    const uint16_t bg = 0x0804;

    lcd.fillRect(boxX, boxY, boxW, boxH, bg);
    lcd.drawRect(boxX, boxY, boxW, boxH, 0x9B15);
    if (hasRelics) {
        lcd.drawText(boxX + 8, boxY + 4,  "PRESS B TO", COLOR_YELLOW, bg, 1);
        lcd.drawText(boxX + 8, boxY + 13, "OFFER RELICS", COLOR_WHITE, bg, 1);
    } else {
        lcd.drawText(boxX + 8, boxY + 4,  "NEED ALIEN", COLOR_YELLOW, bg, 1);
        lcd.drawText(boxX + 8, boxY + 13, "RELICS", COLOR_WHITE, bg, 1);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// drawAltarWeaponRise  �?CG-style smooth laser-gun acquisition animation.
//
// Rendering strategy (mirrors cg.cpp missile trail):
//   �?Background drawn ONCE at animation start; never cleared again.
//   �?SHIP PHASE  : only the ship's bounding box is redrawn when it moves.
//   �?BEAM PHASE  : purely additive �?one new row of pixels per tick.
//                   Nothing is ever erased; the beam trail persists.
//   �?GUN PHASE   : erase only the thin strip the gun just vacated
//                   (redrawPlayRect for 1�? exposed rows + beam restore),
//                   then draw the gun at its new position.
//   Tick gate: 50 ms (�?20 fps), same cadence as CG destruction scene.
// ─────────────────────────────────────────────────────────────────────────────
void drawAltarWeaponRise(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player, uint32_t elapsedMs) {
    static bool     inited    = false;
    static uint32_t lastTick  = 0;
    static int      prevShipY = -1;
    static int      beamTipY  = -1;   // lowest y row already drawn for beam
    static int      prevGunY  = -1;   // top-left y of gun icon on previous tick

    const uint32_t TICK = 50;         // 20 fps gate

    // ── Geometry ─────────────────────────────────────────────────────────────
    const int SX   = 120;                              // ship centre x
    const int SY0  = PLAY_AREA_Y + 2;                 // ship y when entering (top of play area)
    const int SY1  = PLAY_AREA_Y + 28;                // ship y when settled
    const int SBOT = SY1 + 6;                         // ship saucer bottom edge y

    const int WCX  = PLAY_AREA_X + 7 * TILE_SIZE + 8; // beam / well centre x  (128)
    const int WY   = PLAY_AREA_Y + 4 * TILE_SIZE + 10; // well mouth y           (98)

    // Gun left edge �?40-px wide icon centred on WCX
    const int GX   = WCX - 20;                          // 108

    // Three beam centre-x positions (evenly spaced, firing from engine ports)
    const int B1X  = WCX - 12;   // 116
    const int B2X  = WCX;        // 128
    const int B3X  = WCX + 12;   // 140

    // Beam colours
    const uint16_t BOUT  = 0x680D;   // violet outer strand
    const uint16_t BMID  = 0xA81F;   // purple inner strand
    const uint16_t BCORE = 0xFFFF;   // white core

    // ── Detect animation restart (elapsedMs resets to 0 on new trigger) ──────
    if (!inited || elapsedMs < TICK) {
        inited    = false;
        prevShipY = -1;
        beamTipY  = -1;
        prevGunY  = -1;
        lastTick  = 0;
    }

    // ── One-time background draw (frame 0) ────────────────────────────────────
    if (!inited) {
        inited = true;
        drawPlayArea(mapData, player);
        return;   // wait for next tick before starting animation
    }

    // ── Tick gate ─────────────────────────────────────────────────────────────
    uint32_t now = millis();
    if (now - lastTick < TICK) return;
    lastTick = now;

    // ════════════════════════════════════════════════════════════════════════�?
    // PHASE 1 �?Ship descent  (0 �?1000 ms)
    // Only redraws the ship's bounding box when the y-position changes.
    // ════════════════════════════════════════════════════════════════════════�?
    {
        uint32_t p = (elapsedMs < 1000u) ? elapsedMs : 1000u;
        int sy = uiSmoothLerp(SY0, SY1, p, 1000u);
        if (sy != prevShipY) {
            // Erase old position: redraw just the union of old and new ship bbox.
            int oldY  = (prevShipY >= 0) ? prevShipY : SY0;
            int minY  = ((oldY < sy) ? oldY : sy) - 36;
            int maxY  = ((oldY > sy) ? oldY : sy) + 8;
            if (minY < PLAY_AREA_Y) minY = PLAY_AREA_Y;
            redrawPlayRect(mapData, player, SX - 26, minY, 52, maxY - minY + 1);
            drawAltarArrivalShip(SX, sy, true);
            prevShipY = sy;
        }
    }

    // ════════════════════════════════════════════════════════════════════════�?
    // PHASE 2 �?Beam growth  (1000 �?3200 ms, 2200 ms duration)
    // ADDITIVE: each tick draws the next row(s) of beam pixels downward.
    // The old beam is never erased �?it stays drawn, just like a missile trail.
    // ════════════════════════════════════════════════════════════════════════�?
    if (elapsedMs >= 1000u) {
        if (beamTipY < 0) beamTipY = SBOT;   // initialise on first beam tick

        int targetY = (elapsedMs >= 3200u)
                      ? WY
                      : uiSmoothLerp(SBOT, WY, (int)(elapsedMs - 1000u), 2200);

        // Draw only newly exposed rows (additive �?no background clear needed).
        // Three separate 3-px-wide beams: left (B1X), centre (B2X), right (B3X).
        int playBottom = PLAY_AREA_Y + MAP_ROWS * TILE_SIZE - 1;
        for (int y = beamTipY; y <= targetY && y <= playBottom; y++) {
            lcd.drawPixel(B1X - 1, y, BOUT);  lcd.drawPixel(B1X, y, BMID);  lcd.drawPixel(B1X + 1, y, BOUT);
            lcd.drawPixel(B2X - 1, y, BMID);  lcd.drawPixel(B2X, y, BCORE); lcd.drawPixel(B2X + 1, y, BMID);
            lcd.drawPixel(B3X - 1, y, BOUT);  lcd.drawPixel(B3X, y, BMID);  lcd.drawPixel(B3X + 1, y, BOUT);
        }
        beamTipY = targetY + 1;
    }

    // ════════════════════════════════════════════════════════════════════════�?
    // PHASE 3 �?Gun rise  (3200 �?6200 ms, 3000 ms duration)
    // Only erases the thin strip the gun just vacated (1�? rows), then
    // redraws the beam over that strip, then draws the gun at its new position.
    // ════════════════════════════════════════════════════════════════════════�?
    if (elapsedMs >= 3200u) {
        uint32_t rp = elapsedMs - 3200u;
        if (rp > 3000u) rp = 3000u;

        // Gun top-left y: bottom of gun starts at WY, hovers BELOW saucer.
        // GY1 puts the gun 10 px under SBOT so it never overlaps the ship.
        const int GY0 = WY - 11;
        const int GY1 = SBOT + 10;
        int gy = uiSmoothLerp(GY0, GY1, (int)rp, 3000);

        // Hover oscillation once rise is complete.
        if (elapsedMs >= 6200u) gy += ((elapsedMs / 220u) & 1u) ? 1 : 0;

        if (gy != prevGunY) {
            // ── Erase: restore the strip the gun just vacated ────────────────
            if (prevGunY >= 0 && prevGunY > gy) {
                int expTop = gy + 14;          // first exposed row below new gun
                int expBot = prevGunY + 13;    // last row of old gun
                if (expTop <= expBot) {
                    // Restore background tiles for that thin strip (42 px = gun width + margin).
                    redrawPlayRect(mapData, player,
                                   GX - 1, expTop, 42, expBot - expTop + 1);
                    // Repaint all three beams where they intersect the erased strip.
                    for (int ey = expTop; ey <= expBot; ey++) {
                        if (ey >= SBOT && ey <= WY) {
                            lcd.drawPixel(B1X-1,ey,BOUT); lcd.drawPixel(B1X,ey,BMID); lcd.drawPixel(B1X+1,ey,BOUT);
                            lcd.drawPixel(B2X-1,ey,BMID); lcd.drawPixel(B2X,ey,BCORE);lcd.drawPixel(B2X+1,ey,BMID);
                            lcd.drawPixel(B3X-1,ey,BOUT); lcd.drawPixel(B3X,ey,BMID); lcd.drawPixel(B3X+1,ey,BOUT);
                        }
                    }
                }
            }

            // ── Draw: laser-gun icon (40 px wide, 12 px tall, centred on WCX) ─
            // Scope
            lcd.fillRect(GX + 6,  gy,     8, 2, 0x4208);
            lcd.drawPixel(GX + 10, gy,    0xBDF7);          // lens glint
            // Top & bottom rails (full body width)
            lcd.drawFastHLine(GX + 1, gy + 2, 37, 0x6B6D);
            lcd.drawFastHLine(GX + 1, gy + 7, 37, 0x4208);
            // Main body (dark gunmetal, rows 3-6)
            lcd.fillRect(GX + 1,  gy + 3, 20, 4, 0x2945);
            lcd.drawFastHLine(GX + 1, gy + 3, 20, 0x4A69); // top highlight
            // Energy cell (bright magenta/cyan core)
            lcd.fillRect(GX + 21, gy + 2,  8, 6, COLOR_MAGENTA);
            lcd.fillRect(GX + 22, gy + 3,  6, 4, 0xF81F);
            lcd.drawPixel(GX + 24, gy + 2, 0xFFFF);         // cell glint
            // Barrel
            lcd.fillRect(GX + 29, gy + 3,  9, 4, 0x6B6D);
            lcd.drawFastHLine(GX + 29, gy + 3, 9, 0xBDF7); // barrel highlight
            // Muzzle glow (rightmost 2 px)
            lcd.fillRect(GX + 38, gy + 3,  2, 4, COLOR_CYAN);
            lcd.drawPixel(GX + 39, gy + 4, 0xFFFF);
            // Pistol grip (below body)
            lcd.fillRect(GX + 6,  gy + 8,  7, 5, 0x4208);
            lcd.drawFastHLine(GX + 6, gy + 8, 7, 0x8410);  // grip top edge

            prevGunY = gy;
        }
    }

    // ════════════════════════════════════════════════════════════════════════�?
    // PHASE 4 �?Pulse glow  (6200 ms+)
    // Tiny symbol pulse on the ship to signal "done" �?no background repaint.
    // ════════════════════════════════════════════════════════════════════════�?
    if (elapsedMs >= 6200u) {
        uint16_t pc = ((elapsedMs / 300u) & 1u) ? (uint16_t)COLOR_MAGENTA : (uint16_t)0x680Du;
        lcd.fillRect(SX - 1, SY1 - 29, 3, 3, pc);
    }

    // ── legacy names kept below so the rest of the function compiles ─────────
    (void)0;  // suppress "no statements" warning
    const int wellCenterX = WCX;
    const int wellMouthY  = WY;
    const int shipX       = SX;
    (void)shipX;       // used above; kept so any future reference compiles
    (void)wellCenterX;
    (void)wellMouthY;
}
