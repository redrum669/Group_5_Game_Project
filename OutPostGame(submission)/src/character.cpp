#include "character.h"
#include "st7789.h"
#include "config.h"
#include "weapon.h"

// Global flag for corrupted player state
bool playerCorruptedState = false;

// Player color scheme (normal / corrupted mode)
#define PLAYER_ARMOR    (playerCorruptedState ? 0x3006 : 0xC618)
#define PLAYER_SUIT     (playerCorruptedState ? 0x0000 : 0x2945)
#define PLAYER_VISOR    (playerCorruptedState ? 0xF800 : 0x07FF)
#define PLAYER_TRIM     (playerCorruptedState ? 0xA000 : 0xFD20)
#define PLAYER_BOOT     (playerCorruptedState ? 0x0000 : 0x1082)

// Weapon color definitions
#define GUN_COLOR_SMG       0xFFE0
#define GUN_COLOR_MINIGUN   0xFD20
#define GUN_COLOR_LASER     0x07FF
#define GUN_COLOR_FRAME     0x4A69

// 16x16 pixel sprite data
static const uint16_t spriteArmor[2][16] = {
    {
        0x03C0, 0x07E0, 0x07E0, 0x0420,
        0x07E0, 0x03C0, 0x0E70, 0x7FFE,
        0x7FFE, 0x3FFC, 0x3FFC, 0x1FF8,
        0x1E78, 0x1E78, 0x1E78, 0x1E78
    },
    {
        0x03C0, 0x07E0, 0x07E0, 0x0420,
        0x07E0, 0x03C0, 0x0E70, 0x7FFE,
        0x7FFE, 0x3FFC, 0x3FFC, 0x1FF8,
        0x1E78, 0x1E78, 0x1E78, 0x0E38
    }
};

static const uint16_t spriteSuit[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0180, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

static const uint16_t spriteVisor[16] = {
    0x0000, 0x0000, 0x0000,
    0x03C0,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

static const uint16_t spriteTrim[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000,
    0x0180,
    0x0180,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

static const uint16_t spriteBoot[2][16] = {
    {
        0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x1E78, 0x1E78
    },
    {
        0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x1E78, 0x0E38
    }
};

// Weapon sprite data
static const uint16_t weaponSmgBody[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x00C0, 0x01F8, 0x01F0,
    0x03F0, 0x01C0, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

static const uint16_t weaponSmgAccent[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0006, 0x000F,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

static const uint16_t weaponMgBody[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x00A0, 0x07EA, 0x07EA,
    0x0FF0, 0x03C0, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

static const uint16_t weaponMgAccent[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0050, 0x0015, 0x0015,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

static const uint16_t weaponLaserBody[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x01E0, 0x0FEA, 0x0FE0,
    0x0FF0, 0x03C0, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

static const uint16_t weaponLaserAccent[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0010, 0x0015, 0x001F,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

// Character animation state
static int playerDirection = CHAR_DIR_DOWN;
static int playerAnimFrame = 0;

// Recoil animation variables
int recoilTiltStrength = 0;
int recoilMoveDirection = -1;

// Initialize character state
void characterInit() {
    playerDirection = CHAR_DIR_DOWN;
    playerAnimFrame = 0;
}

// Calculate pixel position for sprite scaling
static inline int calculatePixelOffset(int col, int outputSize) {
    return (col * outputSize) >> 4;
}

// Render one sprite layer with recoil distortion
static void renderSingleLayer(int x, int y, const uint16_t* pixelData, uint16_t color, int scale) {
    int renderSize = 8 * scale;
    int centerOffset = (renderSize - 16) / 2;
    int baseX = x - centerOffset;
    int baseY = y - centerOffset;

    for (int row = 0; row < 16; row++) {
        uint16_t rowData = pixelData[row];
        if (rowData == 0) continue;

        int shiftX = 0, shiftY = 0;
        if (recoilTiltStrength > 0) {
            int offsetAmount = ((15 - row) * recoilTiltStrength) / 15;
            if (recoilMoveDirection == CHAR_DIR_LEFT)      shiftX = offsetAmount;
            else if (recoilMoveDirection == CHAR_DIR_RIGHT) shiftX = -offsetAmount;
            else if (recoilMoveDirection == CHAR_DIR_UP)    shiftY = offsetAmount;
            else if (recoilMoveDirection == CHAR_DIR_DOWN)  shiftY = -offsetAmount;
        }

        int yStart = baseY + calculatePixelOffset(row, renderSize) + shiftY;
        int yEnd = baseY + calculatePixelOffset(row + 1, renderSize) + shiftY;
        int height = yEnd - yStart;

        for (int col = 0; col < 16; col++) {
            int bitPos = (playerDirection == CHAR_DIR_LEFT) ? col : (15 - col);
            if ((rowData >> bitPos) & 1) {
                int xLeft = baseX + calculatePixelOffset(col, renderSize) + shiftX;
                int width = calculatePixelOffset(col + 1, renderSize) - calculatePixelOffset(col, renderSize);
                lcd.fillRect(xLeft, yStart, width, height, color);
            }
        }
    }
}

// Draw player character with current weapon
void characterDraw(int x, int y, int weaponID, int scale, uint32_t lastFireTime) {
    int frame = playerAnimFrame;
    const uint16_t* gunBody;
    const uint16_t* gunAccent;
    uint16_t gunColor;

    if (weaponID == WEAPON_MG) {
        gunBody = weaponMgBody;
        gunAccent = weaponMgAccent;
        gunColor = GUN_COLOR_MINIGUN;
    } else if (weaponID == WEAPON_LASER || weaponID == WEAPON_LASER_GUN) {
        gunBody = weaponLaserBody;
        gunAccent = weaponLaserAccent;
        gunColor = GUN_COLOR_LASER;
    } else {
        gunBody = weaponSmgBody;
        gunAccent = weaponSmgAccent;
        gunColor = GUN_COLOR_SMG;
    }

    renderSingleLayer(x, y, spriteSuit, PLAYER_SUIT, scale);
    renderSingleLayer(x, y, spriteArmor[frame], PLAYER_ARMOR, scale);
    renderSingleLayer(x, y, spriteTrim, PLAYER_TRIM, scale);
    renderSingleLayer(x, y, spriteVisor, PLAYER_VISOR, scale);
    renderSingleLayer(x, y, spriteBoot[frame], PLAYER_BOOT, scale);

    renderSingleLayer(x, y, gunBody, GUN_COLOR_FRAME, scale);
    renderSingleLayer(x, y, gunAccent, gunColor, scale);

    // Draw laser blade if inactive
    if (weaponID == WEAPON_LASER && (lastFireTime == 0 || millis() - lastFireTime >= 300)) {
        int bladeLength = 10 * scale;
        int bladeHeight = 2 * scale;
        int bladeY = y + 3 * scale;
        int coreHeight = (bladeHeight / 2 > 0) ? (bladeHeight / 2) : 1;

        if (playerDirection == CHAR_DIR_LEFT) {
            int startX = x - bladeLength + 3 * scale;
            lcd.fillRect(startX, bladeY, bladeLength, bladeHeight, COLOR_CYAN);
            lcd.fillRect(startX + 2, bladeY + (scale > 1 ? 1 : 0), bladeLength - 4, coreHeight, COLOR_WHITE);
        } else {
            int startX = x + 5 * scale;
            lcd.fillRect(startX, bladeY, bladeLength, bladeHeight, COLOR_CYAN);
            lcd.fillRect(startX + 2, bladeY + (scale > 1 ? 1 : 0), bladeLength - 4, coreHeight, COLOR_WHITE);
        }
    }
}

// Draw character with solid background (opaque mode)
void characterDrawOpaque(int x, int y, int weaponID, uint16_t bgColor, int scale, uint32_t lastFireTime) {
    int frame = playerAnimFrame;
    const uint16_t* suitData  = spriteSuit;
    const uint16_t* armorData = spriteArmor[frame];
    const uint16_t* trimData  = spriteTrim;
    const uint16_t* visorData = spriteVisor;
    const uint16_t* bootData  = spriteBoot[frame];

    const uint16_t* gunBody;
    const uint16_t* gunAccent;
    uint16_t gunColor;

    if (weaponID == WEAPON_MG) {
        gunBody = weaponMgBody;
        gunAccent = weaponMgAccent;
        gunColor = GUN_COLOR_MINIGUN;
    } else if (weaponID == WEAPON_LASER || weaponID == WEAPON_LASER_GUN) {
        gunBody = weaponLaserBody;
        gunAccent = weaponLaserAccent;
        gunColor = GUN_COLOR_LASER;
    } else {
        gunBody = weaponSmgBody;
        gunAccent = weaponSmgAccent;
        gunColor = GUN_COLOR_SMG;
    }

    int renderSize = 8 * scale;
    int centerOffset = (renderSize - 16) / 2;
    int baseX = x - centerOffset;
    int baseY = y - centerOffset;

    for (int row = 0; row < 16; row++) {
        uint16_t suitRow = suitData[row];
        uint16_t armorRow = armorData[row];
        uint16_t trimRow = trimData[row];
        uint16_t visorRow = visorData[row];
        uint16_t bootRow = bootData[row];
        uint16_t gunBodyRow = gunBody[row];
        uint16_t gunAccentRow = gunAccent[row];

        int yStart = baseY + calculatePixelOffset(row, renderSize);
        int height = calculatePixelOffset(row + 1, renderSize) - calculatePixelOffset(row, renderSize);

        for (int col = 0; col < 16; col++) {
            int bitPos = (playerDirection == CHAR_DIR_LEFT) ? col : (15 - col);
            uint16_t mask = (uint16_t)(1u << bitPos);
            uint16_t finalColor;

            if (gunAccentRow & mask) finalColor = gunColor;
            else if (gunBodyRow & mask) finalColor = GUN_COLOR_FRAME;
            else if (bootRow & mask) finalColor = PLAYER_BOOT;
            else if (visorRow & mask) finalColor = PLAYER_VISOR;
            else if (trimRow & mask) finalColor = PLAYER_TRIM;
            else if (armorRow & mask) finalColor = PLAYER_ARMOR;
            else if (suitRow & mask) finalColor = PLAYER_SUIT;
            else finalColor = bgColor;

            int xLeft = baseX + calculatePixelOffset(col, renderSize);
            int width = calculatePixelOffset(col + 1, renderSize) - calculatePixelOffset(col, renderSize);
            lcd.fillRect(xLeft, yStart, width, height, finalColor);
        }
    }

    // Draw laser blade
    if (weaponID == WEAPON_LASER && (lastFireTime == 0 || millis() - lastFireTime >= 300)) {
        int bladeLength = 10 * scale;
        int bladeHeight = 2 * scale;
        int bladeY = y + 3 * scale;
        int coreHeight = (bladeHeight / 2 > 0) ? (bladeHeight / 2) : 1;

        if (playerDirection == CHAR_DIR_LEFT) {
            int startX = x - bladeLength + 3 * scale;
            lcd.fillRect(startX, bladeY, bladeLength, bladeHeight, COLOR_CYAN);
            lcd.fillRect(startX + 2, bladeY + (scale > 1 ? 1 : 0), bladeLength - 4, coreHeight, COLOR_WHITE);
        } else {
            int startX = x + 5 * scale;
            lcd.fillRect(startX, bladeY, bladeLength, bladeHeight, COLOR_CYAN);
            lcd.fillRect(startX + 2, bladeY + (scale > 1 ? 1 : 0), bladeLength - 4, coreHeight, COLOR_WHITE);
        }
    }
}

// Set character facing direction
void characterSetDir(int dir) {
    playerDirection = dir;
    playerAnimFrame ^= 1;
}

// Get current facing direction
int characterGetDir() {
    return playerDirection;
}

// Draw farmer NPC
void characterDrawFarmer(int x, int y) {
    const uint16_t skinColor = 0xFC10;
    const uint16_t hairColor = 0x6180;
    const uint16_t shirtColor = 0x03E0;
    const uint16_t pantsColor = 0x212C;
    const uint16_t toolHandle = 0x8A84;
    const uint16_t toolBlade = 0xD6BA;

    lcd.drawFastVLine(x + 2, y + 4, 18, toolHandle);
    lcd.drawFastHLine(x - 1, y + 4, 5, toolBlade);
    lcd.drawPixel(x - 2, y + 5, toolBlade);
    lcd.drawPixel(x - 2, y + 6, toolBlade);
    lcd.drawPixel(x - 1, y + 7, toolBlade);

    lcd.fillRect(x + 4, y + 1, 8, 3, hairColor);
    lcd.fillRect(x + 3, y + 4, 10, 8, skinColor);
    lcd.fillRect(x + 4, y + 3, 8, 2, hairColor);
    lcd.drawPixel(x + 5, y + 7, COLOR_BLACK);
    lcd.drawPixel(x + 10, y + 7, COLOR_BLACK);
    lcd.drawFastHLine(x + 6, y + 10, 3, 0xA800);

    lcd.fillRect(x + 4, y + 12, 8, 8, shirtColor);
    lcd.fillRect(x + 2, y + 13, 2, 6, skinColor);
    lcd.fillRect(x + 12, y + 13, 2, 6, skinColor);
    lcd.fillRect(x + 5, y + 20, 3, 5, pantsColor);
    lcd.fillRect(x + 8, y + 20, 3, 5, pantsColor);
    lcd.drawFastHLine(x + 4, y + 25, 3, 0x0000);
    lcd.drawFastHLine(x + 8, y + 25, 3, 0x0000);
}

// Draw merchant NPC
void characterDrawMerchant(int x, int y) {
    const uint16_t robeColor = 0xA145;
    const uint16_t robeDarkColor = 0x7924;
    const uint16_t trimColor = 0xFDE0;
    const uint16_t skinColor = 0xE69A;
    const uint16_t hoodColor = 0x39CE;
    const uint16_t glowColor = 0x07FF;

    lcd.drawPixel(x + 4, y + 1, glowColor);
    lcd.drawPixel(x + 11, y + 1, glowColor);
    lcd.fillRect(x + 4, y + 2, 8, 3, hoodColor);
    lcd.fillRect(x + 5, y + 5, 6, 3, skinColor);
    lcd.drawPixel(x + 6, y + 6, COLOR_BLACK);
    lcd.drawPixel(x + 9, y + 6, COLOR_BLACK);
    lcd.fillRect(x + 3, y + 8, 10, 5, robeColor);
    lcd.fillRect(x + 2, y + 12, 12, 3, robeDarkColor);
    lcd.drawFastHLine(x + 4, y + 8, 8, trimColor);
    lcd.drawFastVLine(x + 7, y + 9, 4, trimColor);
    lcd.fillRect(x + 1, y + 9, 2, 3, skinColor);
    lcd.fillRect(x + 13, y + 9, 2, 3, skinColor);
    lcd.fillRect(x + 2, y + 10, 2, 4, 0x734D);
    lcd.fillRect(x + 12, y + 10, 2, 4, 0x734D);
    lcd.fillRect(x + 5, y + 14, 2, 2, 0x4208);
    lcd.fillRect(x + 9, y + 14, 2, 2, 0x4208);
}

// Draw high priest NPC
void characterDrawPriest(int x, int y) {
    uint16_t cloakColor = 0x300A;
    uint16_t foldColor = 0x780F;
    uint16_t trimColor = 0xA81F;
    uint16_t auraColor = 0x680D;

    lcd.drawPixel(x + 2, y + 2, auraColor);
    lcd.drawPixel(x + 13, y + 1, auraColor);
    lcd.drawPixel(x + 1, y + 6, auraColor);
    lcd.drawPixel(x + 14, y + 7, auraColor);
    lcd.drawPixel(x + 3, y + 15, auraColor);
    lcd.drawPixel(x + 12, y + 15, auraColor);
    lcd.fillRect(x + 4, y + 0, 8, 3, cloakColor);
    lcd.fillRect(x + 3, y + 3, 10, 9, cloakColor);
    lcd.fillRect(x + 2, y + 12, 12, 4, cloakColor);
    lcd.drawFastHLine(x + 3, y + 3, 10, trimColor);
    lcd.drawFastHLine(x + 2, y + 12, 12, 0x580C);
    lcd.drawFastVLine(x + 4, y + 5, 9, foldColor);
    lcd.drawFastVLine(x + 11, y + 5, 9, foldColor);
    lcd.fillRect(x + 5, y + 4, 6, 6, COLOR_BLACK);
    lcd.drawPixel(x + 7, y + 5, 0xF81F);
    lcd.drawPixel(x + 8, y + 5, 0xF81F);
    lcd.drawFastVLine(x + 5, y + 12, 4, 0x8410);
    lcd.drawFastVLine(x + 10, y + 12, 4, 0x8410);
    lcd.drawFastVLine(x + 13, y + 2, 11, 0xD6BA);
    lcd.drawFastHLine(x + 12, y + 3, 4, 0xD6BA);
    lcd.fillRect(x + 12, y + 0, 3, 3, 0xA81F);
    lcd.drawPixel(x + 13, y + 0, 0xF81F);
    lcd.drawPixel(x + 11, y + 1, auraColor);
    lcd.drawPixel(x + 15, y + 1, auraColor);
}

// Draw skull decoration
void characterDrawSkull(int x, int y) {
    lcd.fillRect(x + 2, y + 3, 12, 8, 0xD6BA);
    lcd.fillRect(x + 3, y + 10, 10, 3, 0xC618);
    lcd.drawFastHLine(x + 4, y + 2, 8, 0xFFFF);
    lcd.drawPixel(x + 3, y + 3, 0xFFFF);
    lcd.drawPixel(x + 12, y + 3, 0xFFFF);
    lcd.fillRect(x + 4, y + 5, 3, 4, COLOR_BLACK);
    lcd.fillRect(x + 9, y + 5, 3, 4, COLOR_BLACK);
    lcd.fillRect(x + 7, y + 8, 2, 2, COLOR_BLACK);
    lcd.drawFastHLine(x + 5, y + 11, 6, 0x8410);
    lcd.drawFastVLine(x + 6, y + 11, 3, 0x9CF3);
    lcd.drawFastVLine(x + 9, y + 11, 3, 0x9CF3);
    lcd.drawPixel(x + 2, y + 2, 0xF81F);
    lcd.drawPixel(x + 13, y + 2, 0xA81F);
}

// Draw red-visor scout enemy
void characterDrawRedVisorScout(int x, int y) {
    const uint16_t armorColor = 0xA534;
    const uint16_t suitColor = 0x39A6;
    const uint16_t visorColor = 0xF800;
    const uint16_t packColor = 0x9A60;
    const uint16_t bootColor = 0x2104;

    lcd.fillRect(x + 4, y + 1, 8, 3, armorColor);
    lcd.fillRect(x + 3, y + 4, 10, 4, armorColor);
    lcd.fillRect(x + 4, y + 8, 8, 6, suitColor);
    lcd.fillRect(x + 2, y + 9, 2, 4, armorColor);
    lcd.fillRect(x + 12, y + 9, 2, 4, armorColor);
    lcd.fillRect(x + 4, y + 14, 3, 2, armorColor);
    lcd.fillRect(x + 9, y + 14, 3, 2, armorColor);
    lcd.fillRect(x + 4, y + 16, 2, 6, suitColor);
    lcd.fillRect(x + 10, y + 16, 2, 6, suitColor);
    lcd.fillRect(x + 4, y + 22, 3, 2, bootColor);
    lcd.fillRect(x + 9, y + 22, 3, 2, bootColor);
    lcd.fillRect(x + 3, y + 18, 2, 3, packColor);
    lcd.fillRect(x + 11, y + 18, 2, 3, packColor);
    lcd.fillRect(x + 4, y + 5, 8, 2, visorColor);
    lcd.drawPixel(x + 5, y + 5, COLOR_WHITE);
    lcd.drawPixel(x + 10, y + 5, COLOR_WHITE);
    lcd.fillRect(x + 13, y + 10, 3, 2, COLOR_LIGHTGRAY);
}

// Draw civilian NPC
void characterDrawCivilian(int x, int y) {
    const uint16_t skinColor = 0xFD33;
    const uint16_t hairColor = 0x49A0;
    const uint16_t coatColor = 0x738E;
    const uint16_t shirtColor = 0xFFE0;
    const uint16_t pantsColor = 0x3186;
    const uint16_t shoeColor = 0x2104;

    lcd.fillRect(x + 4, y + 1, 8, 3, hairColor);
    lcd.fillRect(x + 3, y + 4, 10, 7, skinColor);
    lcd.fillRect(x + 4, y + 3, 8, 2, hairColor);
    lcd.drawPixel(x + 5, y + 7, COLOR_BLACK);
    lcd.drawPixel(x + 10, y + 7, COLOR_COMMERCIAL_BLACK);
    lcd.drawPixel(x + 7, y + 9, 0xA800);
    lcd.fillRect(x + 4, y + 11, 8, 7, coatColor);
    lcd.fillRect(x + 3, y + 17, 10, 2, coatColor);
    lcd.fillRect(x + 5, y + 12, 6, 3, shirtColor);
    lcd.fillRect(x + 2, y + 12, 2, 6, skinColor);
    lcd.fillRect(x + 12, y + 12, 2, 6, skinColor);
    lcd.fillRect(x + 4, y + 19, 3, 6, pantsColor);
    lcd.fillRect(x + 9, y + 19, 3, 6, pantsColor);
    lcd.drawFastVLine(x + 7, y + 19, 6, COLOR_BLACK);
    lcd.fillRect(x + 3, y + 25, 4, 2, shoeColor);
    lcd.fillRect(x + 9, y + 25, 4, 2, shoeColor);
}

// Draw wizard NPC
void characterDrawWizard(int x, int y) {
    const uint16_t robeColor = 0x500F;
    const uint16_t robeDarkColor = 0x2808;
    const uint16_t trimColor = 0xA81F;
    const uint16_t auraColor = 0x680D;
    const uint16_t staffColor = 0xB596;
    const uint16_t bootColor = 0x2104;

    lcd.drawPixel(x + 2, y + 2, auraColor);
    lcd.drawPixel(x + 13, y + 2, auraColor);
    lcd.drawPixel(x + 1, y + 8, auraColor);
    lcd.drawPixel(x + 14, y + 8, auraColor);
    lcd.fillRect(x + 4, y + 1, 8, 3, robeColor);
    lcd.fillRect(x + 3, y + 4, 10, 10, robeColor);
    lcd.fillRect(x + 2, y + 14, 12, 3, robeDarkColor);
    lcd.drawFastHLine(x + 3, y + 4, 10, trimColor);
    lcd.drawFastVLine(x + 5, y + 7, 8, robeDarkColor);
    lcd.drawFastVLine(x + 10, y + 7, 8, robeDarkColor);
    lcd.fillRect(x + 5, y + 5, 6, 5, COLOR_BLACK);
    lcd.drawPixel(x + 7, y + 6, COLOR_MAGENTA);
    lcd.drawPixel(x + 8, y + 6, COLOR_MAGENTA);
    lcd.fillRect(x + 4, y + 17, 2, 5, robeDarkColor);
    lcd.fillRect(x + 10, y + 17, 2, 5, robeDarkColor);
    lcd.fillRect(x + 4, y + 22, 3, 2, bootColor);
    lcd.fillRect(x + 9, y + 22, 3, 2, bootColor);
    lcd.drawFastVLine(x + 13, y + 3, 11, staffColor);
    lcd.fillRect(x + 12, y + 1, 3, 3, COLOR_CYAN);
    lcd.drawPixel(x + 13, y + 0, COLOR_WHITE);
}