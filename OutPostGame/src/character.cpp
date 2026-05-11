#include "character.h"
#include "st7789.h"
#include "config.h"
#include "weapon.h"

bool g_playerCorrupted = false;

// ── Hardcore Sci-Fi Space Marine palette ────────────────────────────
#define CHAR_ARMOR  (g_playerCorrupted ? 0x3006 : 0xC618)   // steel plating (light grey) or dark purple (corrupted)
#define CHAR_SUIT   (g_playerCorrupted ? 0x0000 : 0x2945)   // inner body suit (dark slate) or black (corrupted)
#define CHAR_VISOR  (g_playerCorrupted ? 0xF800 : 0x07FF)   // glowing cyan or red visor (corrupted)
#define CHAR_TRIM   (g_playerCorrupted ? 0xA000 : 0xFD20)   // orange rim / emblem or dark red (corrupted)
#define CHAR_BOOT   (g_playerCorrupted ? 0x0000 : 0x1082)   // near-black boots

#define CHAR_GUN_SMG   0xFFE0   // yellow SMG
#define CHAR_GUN_MG    0xFD20   // orange minigun
#define CHAR_GUN_LASER 0x07FF   // cyan laser rifle
#define CHAR_GUN_BODY  0x4A69   // dark metallic grey gun body

// ── Sprite bitmaps (16×16, MSB = leftmost pixel) ────────────────────
// Sleek heroic V-taper silhouette: head 8w, shoulders 14w, chest 12w,
// waist 10w, legs 4+4w with 2-col inner gap. Rendered at scale=2 → 16 px
// (safe zones) or scale=3 → 24 px (combat zones).
static const uint16_t armor_layer[2][16] = {
    {
        0x03C0,  // row 0   ......xxxx......   crown (4w)
        0x07E0,  // row 1   .....xxxxxx.....   helmet (6w)
        0x07E0,  // row 2   .....xxxxxx.....
        0x0420,  // row 3   .....x....x.....   visor slot (sides only)
        0x07E0,  // row 4   .....xxxxxx.....
        0x03C0,  // row 5   ......xxxx......   jaw narrows (4w)
        0x0E70,  // row 6   ....xxx..xxx....   collar (suit shows between)
        0x7FFE,  // row 7   .xxxxxxxxxxxxxx.   shoulder pads (14w)
        0x7FFE,  // row 8   .xxxxxxxxxxxxxx.   chest top
        0x3FFC,  // row 9   ..xxxxxxxxxxxx..   chest taper (emblem overlay)
        0x3FFC,  // row 10  ..xxxxxxxxxxxx..
        0x1FF8,  // row 11  ...xxxxxxxxxx...   waist/belt
        0x1E78,  // row 12  ...xxxx..xxxx...   thighs (4+4w, 2-col gap)
        0x1E78,  // row 13
        0x1E78,  // row 14  (boots overlay)
        0x1E78,  // row 15  frame 0 — both feet planted
    },
    {
        0x03C0, 0x07E0, 0x07E0, 0x0420,
        0x07E0, 0x03C0, 0x0E70, 0x7FFE,
        0x7FFE, 0x3FFC, 0x3FFC, 0x1FF8,
        0x1E78, 0x1E78, 0x1E78,
        0x0E38,   // frame 1 — left foot pulled back, right foot forward
    },
};

// Inner body suit: visible through collar gap only. Legs leave a clean
// silhouette so the marine doesn't look stocky in the combat zone.
static const uint16_t suit_layer[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0180, 0x0000,   // row 6: neck column (cols 7,8)
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};

// Cyan visor band fills the helmet slot at row 3 (4w, cols 6-9).
static const uint16_t visor_layer[16] = {
    0x0000, 0x0000, 0x0000,
    0x03C0,  // row 3  ......xxxx......   visor (cols 6-9)
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};

// Orange chest emblem: small vertical bar on the sternum.
static const uint16_t trim_layer[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000,
    0x0180,  // row 9   .......xx.......   emblem (cols 7,8)
    0x0180,  // row 10
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

// Boots: darker overlay on the last two rows, same silhouette as legs.
static const uint16_t boot_layer[2][16] = {
    {
        0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x1E78, 0x1E78,
    },
    {
        0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x1E78, 0x0E38,
    },
};

// Weapons held at the right hip, muzzle pointing up/right.
static const uint16_t weapon_smg_body[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x00C0, 0x01F8, 0x01F0,   // extended barrel and sight
    0x03F0, 0x01C0, 0x0000, 0x0000,   // stock and grip
    0x0000, 0x0000, 0x0000, 0x0000,
};
static const uint16_t weapon_smg_accent[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0006, 0x000F,   // yellow tip/accent
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};

static const uint16_t weapon_mg_body[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x00A0, 0x07EA, 0x07EA,   // long heavy barrel body
    0x0FF0, 0x03C0, 0x0000, 0x0000,   // bulky stock
    0x0000, 0x0000, 0x0000, 0x0000,
};
static const uint16_t weapon_mg_accent[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0050, 0x0015, 0x0015,   // orange stripes/tip
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};

static const uint16_t weapon_laser_body[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x01E0, 0x0FEA, 0x0FE0,   // sleek emitter body
    0x0FF0, 0x03C0, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};
static const uint16_t weapon_laser_accent[16] = {
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0010, 0x0015, 0x001F,   // cyan glow stripes
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};

static int charDir   = CHAR_DIR_DOWN;
static int charFrame = 0;

void characterInit() {
    charDir   = CHAR_DIR_DOWN;
    charFrame = 0;
}

// Map input-bit column c (0..16) to an output-pixel offset for any outSize.
// Used so a 16×16 bitmap renders cleanly at 16 px (1:1) or 24 px (1.5×,
// alternating 1/2-px block widths).
static inline int bitOffset(int c, int outSize) { return (c * outSize) >> 4; }

int g_recoilTiltAmount = 0;
int g_recoilDir = -1;

static void drawLayer(int px, int py, const uint16_t* data, uint16_t color, int scale) {
    int outSize = 8 * scale;            // 16 or 24
    int off     = (outSize - 16) / 2;   // centre on the 16×16 tile anchor
    int bx      = px - off;
    int by      = py - off;
    for (int row = 0; row < 16; row++) {
        uint16_t bits = data[row];
        if (!bits) continue;
        int tiltX = 0, tiltY = 0;
        if (g_recoilTiltAmount > 0) {
            int shift = ((15 - row) * g_recoilTiltAmount) / 15;
            if (g_recoilDir == CHAR_DIR_LEFT) tiltX = shift;
            else if (g_recoilDir == CHAR_DIR_RIGHT) tiltX = -shift;
            else if (g_recoilDir == CHAR_DIR_UP) tiltY = shift;
            else if (g_recoilDir == CHAR_DIR_DOWN) tiltY = -shift;
        }
        int yTop = by + bitOffset(row, outSize) + tiltY;
        int yBot = by + bitOffset(row + 1, outSize) + tiltY;
        int h    = yBot - yTop;
        for (int col = 0; col < 16; col++) {
            int bitIndex = (charDir == CHAR_DIR_LEFT) ? col : (15 - col);
            if ((bits >> bitIndex) & 1) {
                int xL = bx + bitOffset(col, outSize) + tiltX;
                int w  = bitOffset(col + 1, outSize) - bitOffset(col, outSize);
                lcd.fillRect(xL, yTop, w, h, color);
            }
        }
    }
}

void characterDraw(int px, int py, int weaponType, int scale, uint32_t lastFire) {
    int f = charFrame;
    const uint16_t* wep_body;
    const uint16_t* wep_accent;
    uint16_t wepCol;
    if (weaponType == WEAPON_MG)         { wep_body = weapon_mg_body;    wep_accent = weapon_mg_accent;    wepCol = CHAR_GUN_MG;    }
    else if (weaponType == WEAPON_LASER || weaponType == WEAPON_LASER_GUN) { wep_body = weapon_laser_body; wep_accent = weapon_laser_accent; wepCol = CHAR_GUN_LASER; }
    else                                 { wep_body = weapon_smg_body;   wep_accent = weapon_smg_accent;   wepCol = CHAR_GUN_SMG;   }

    drawLayer(px, py, suit_layer,       CHAR_SUIT,  scale);
    drawLayer(px, py, armor_layer[f],   CHAR_ARMOR, scale);
    drawLayer(px, py, trim_layer,       CHAR_TRIM,  scale);
    drawLayer(px, py, visor_layer,      CHAR_VISOR, scale);
    drawLayer(px, py, boot_layer[f],    CHAR_BOOT,  scale);

    drawLayer(px, py, wep_body,         CHAR_GUN_BODY, scale);
    drawLayer(px, py, wep_accent,       wepCol,     scale);

    if (weaponType == WEAPON_LASER && (lastFire == 0 || millis() - lastFire >= 300)) {
        int bladeLen = 10 * scale; // 30 pixels long blade
        int h = 2 * scale;
        int yTop = py + 3 * scale;
        int coreH = (h / 2 > 0) ? (h / 2) : 1;
        if (charDir == CHAR_DIR_LEFT) {
            int startX = px - bladeLen + 3 * scale;
            lcd.fillRect(startX, yTop, bladeLen, h, COLOR_CYAN);
            lcd.fillRect(startX + 2, yTop + (scale > 1 ? 1 : 0), bladeLen - 4, coreH, COLOR_WHITE);
        } else {
            int startX = px + 5 * scale;
            lcd.fillRect(startX, yTop, bladeLen, h, COLOR_CYAN);
            lcd.fillRect(startX + 2, yTop + (scale > 1 ? 1 : 0), bladeLen - 4, coreH, COLOR_WHITE);
        }
    }
}

void characterDrawOpaque(int px, int py, int weaponType, uint16_t bg, int scale, uint32_t lastFire) {
    int f = charFrame;
    const uint16_t* suit  = suit_layer;
    const uint16_t* armor = armor_layer[f];
    const uint16_t* trim  = trim_layer;
    const uint16_t* visor = visor_layer;
    const uint16_t* boot  = boot_layer[f];
    const uint16_t* wep_body;
    const uint16_t* wep_accent;
    uint16_t wepCol;
    if (weaponType == WEAPON_MG)         { wep_body = weapon_mg_body;    wep_accent = weapon_mg_accent;    wepCol = CHAR_GUN_MG;    }
    else if (weaponType == WEAPON_LASER || weaponType == WEAPON_LASER_GUN) { wep_body = weapon_laser_body; wep_accent = weapon_laser_accent; wepCol = CHAR_GUN_LASER; }
    else                                 { wep_body = weapon_smg_body;   wep_accent = weapon_smg_accent;   wepCol = CHAR_GUN_SMG;   }

    int outSize = 8 * scale;
    int off     = (outSize - 16) / 2;
    int bx      = px - off;
    int by      = py - off;
    for (int row = 0; row < 16; row++) {
        uint16_t sr = suit[row], ar = armor[row], tr = trim[row];
        uint16_t vr = visor[row], br = boot[row];
        uint16_t wb = wep_body[row], wa = wep_accent[row];
        int yTop = by + bitOffset(row, outSize);
        int h    = bitOffset(row + 1, outSize) - bitOffset(row, outSize);
        for (int col = 0; col < 16; col++) {
            int bitIndex = (charDir == CHAR_DIR_LEFT) ? col : (15 - col);
            uint16_t mask = (uint16_t)(1u << bitIndex);
            uint16_t c;
            if (wa & mask)      c = wepCol;
            else if (wb & mask) c = CHAR_GUN_BODY;
            else if (br & mask) c = CHAR_BOOT;
            else if (vr & mask) c = CHAR_VISOR;
            else if (tr & mask) c = CHAR_TRIM;
            else if (ar & mask) c = CHAR_ARMOR;
            else if (sr & mask) c = CHAR_SUIT;
            else                c = bg;
            int xL = bx + bitOffset(col, outSize);
            int w  = bitOffset(col + 1, outSize) - bitOffset(col, outSize);
            lcd.fillRect(xL, yTop, w, h, c);
        }
    }

    if (weaponType == WEAPON_LASER && (lastFire == 0 || millis() - lastFire >= 300)) {
        int bladeLen = 10 * scale; // 30 pixels long blade
        int h = 2 * scale;
        int yTop = py + 3 * scale;
        int coreH = (h / 2 > 0) ? (h / 2) : 1;
        if (charDir == CHAR_DIR_LEFT) {
            int startX = px - bladeLen + 3 * scale;
            lcd.fillRect(startX, yTop, bladeLen, h, COLOR_CYAN);
            lcd.fillRect(startX + 2, yTop + (scale > 1 ? 1 : 0), bladeLen - 4, coreH, COLOR_WHITE);
        } else {
            int startX = px + 5 * scale;
            lcd.fillRect(startX, yTop, bladeLen, h, COLOR_CYAN);
            lcd.fillRect(startX + 2, yTop + (scale > 1 ? 1 : 0), bladeLen - 4, coreH, COLOR_WHITE);
        }
    }
}

void characterSetDir(int dir) {
    charDir    = dir;
    charFrame ^= 1;
}

int characterGetDir() {
    return charDir;
}

void characterDrawFarmer(int px, int py) {
    const uint16_t skin = 0xFC10;
    const uint16_t hair = 0x6180;
    const uint16_t shirt = 0x03E0;
    const uint16_t pants = 0x212C;
    const uint16_t handle = 0x8A84;
    const uint16_t blade = 0xD6BA;

    lcd.drawFastVLine(px + 2, py + 4, 18, handle);
    lcd.drawFastHLine(px - 1, py + 4, 5, blade);
    lcd.drawPixel(px - 2, py + 5, blade);
    lcd.drawPixel(px - 2, py + 6, blade);
    lcd.drawPixel(px - 1, py + 7, blade);

    lcd.fillRect(px + 4, py + 1, 8, 3, hair);
    lcd.fillRect(px + 3, py + 4, 10, 8, skin);
    lcd.fillRect(px + 4, py + 3, 8, 2, hair);
    lcd.drawPixel(px + 5, py + 7, COLOR_BLACK);
    lcd.drawPixel(px + 10, py + 7, COLOR_BLACK);
    lcd.drawFastHLine(px + 6, py + 10, 3, 0xA800);

    lcd.fillRect(px + 4, py + 12, 8, 8, shirt);
    lcd.fillRect(px + 2, py + 13, 2, 6, skin);
    lcd.fillRect(px + 12, py + 13, 2, 6, skin);
    lcd.fillRect(px + 5, py + 20, 3, 5, pants);
    lcd.fillRect(px + 8, py + 20, 3, 5, pants);
    lcd.drawFastHLine(px + 4, py + 25, 3, 0x0000);
    lcd.drawFastHLine(px + 8, py + 25, 3, 0x0000);
}

void characterDrawMerchant(int px, int py) {
    const uint16_t robe = 0xA145;
    const uint16_t robeDark = 0x7924;
    const uint16_t trim = 0xFDE0;
    const uint16_t skin = 0xE69A;
    const uint16_t hood = 0x39CE;
    const uint16_t glow = 0x07FF;

    lcd.drawPixel(px + 4, py + 1, glow);
    lcd.drawPixel(px + 11, py + 1, glow);
    lcd.fillRect(px + 4, py + 2, 8, 3, hood);
    lcd.fillRect(px + 5, py + 5, 6, 3, skin);
    lcd.drawPixel(px + 6, py + 6, COLOR_BLACK);
    lcd.drawPixel(px + 9, py + 6, COLOR_BLACK);
    lcd.fillRect(px + 3, py + 8, 10, 5, robe);
    lcd.fillRect(px + 2, py + 12, 12, 3, robeDark);
    lcd.drawFastHLine(px + 4, py + 8, 8, trim);
    lcd.drawFastVLine(px + 7, py + 9, 4, trim);
    lcd.fillRect(px + 1, py + 9, 2, 3, skin);
    lcd.fillRect(px + 13, py + 9, 2, 3, skin);
    lcd.fillRect(px + 2, py + 10, 2, 4, 0x734D);
    lcd.fillRect(px + 12, py + 10, 2, 4, 0x734D);
    lcd.fillRect(px + 5, py + 14, 2, 2, 0x4208);
    lcd.fillRect(px + 9, py + 14, 2, 2, 0x4208);
}

void characterDrawPriest(int px, int py) {
    uint16_t cloak = 0x300A;
    uint16_t fold = 0x780F;
    uint16_t trim = 0xA81F;
    uint16_t aura = 0x680D;

    lcd.drawPixel(px + 2, py + 2, aura);
    lcd.drawPixel(px + 13, py + 1, aura);
    lcd.drawPixel(px + 1, py + 6, aura);
    lcd.drawPixel(px + 14, py + 7, aura);
    lcd.drawPixel(px + 3, py + 15, aura);
    lcd.drawPixel(px + 12, py + 15, aura);
    lcd.fillRect(px + 4, py + 0, 8, 3, cloak);
    lcd.fillRect(px + 3, py + 3, 10, 9, cloak);
    lcd.fillRect(px + 2, py + 12, 12, 4, cloak);
    lcd.drawFastHLine(px + 3, py + 3, 10, trim);
    lcd.drawFastHLine(px + 2, py + 12, 12, 0x580C);
    lcd.drawFastVLine(px + 4, py + 5, 9, fold);
    lcd.drawFastVLine(px + 11, py + 5, 9, fold);
    lcd.fillRect(px + 5, py + 4, 6, 6, COLOR_BLACK);
    lcd.drawPixel(px + 7, py + 5, 0xF81F);
    lcd.drawPixel(px + 8, py + 5, 0xF81F);
    lcd.drawFastVLine(px + 5, py + 12, 4, 0x8410);
    lcd.drawFastVLine(px + 10, py + 12, 4, 0x8410);
    lcd.drawFastVLine(px + 13, py + 2, 11, 0xD6BA);
    lcd.drawFastHLine(px + 12, py + 3, 4, 0xD6BA);
    lcd.fillRect(px + 12, py + 0, 3, 3, 0xA81F);
    lcd.drawPixel(px + 13, py + 0, 0xF81F);
    lcd.drawPixel(px + 11, py + 1, aura);
    lcd.drawPixel(px + 15, py + 1, aura);
}

void characterDrawSkull(int px, int py) {
    lcd.fillRect(px + 2, py + 3, 12, 8, 0xD6BA);
    lcd.fillRect(px + 3, py + 10, 10, 3, 0xC618);
    lcd.drawFastHLine(px + 4, py + 2, 8, 0xFFFF);
    lcd.drawPixel(px + 3, py + 3, 0xFFFF);
    lcd.drawPixel(px + 12, py + 3, 0xFFFF);
    lcd.fillRect(px + 4, py + 5, 3, 4, COLOR_BLACK);
    lcd.fillRect(px + 9, py + 5, 3, 4, COLOR_BLACK);
    lcd.fillRect(px + 7, py + 8, 2, 2, COLOR_BLACK);
    lcd.drawFastHLine(px + 5, py + 11, 6, 0x8410);
    lcd.drawFastVLine(px + 6, py + 11, 3, 0x9CF3);
    lcd.drawFastVLine(px + 9, py + 11, 3, 0x9CF3);
    lcd.drawPixel(px + 2, py + 2, 0xF81F);
    lcd.drawPixel(px + 13, py + 2, 0xA81F);
}

void characterDrawRedVisorScout(int px, int py) {
    const uint16_t armor = 0xA534;
    const uint16_t suit = 0x39A6;
    const uint16_t visor = 0xF800;
    const uint16_t pack = 0x9A60;
    const uint16_t boot = 0x2104;

    lcd.fillRect(px + 4, py + 1, 8, 3, armor);
    lcd.fillRect(px + 3, py + 4, 10, 4, armor);
    lcd.fillRect(px + 4, py + 8, 8, 6, suit);
    lcd.fillRect(px + 2, py + 9, 2, 4, armor);
    lcd.fillRect(px + 12, py + 9, 2, 4, armor);
    lcd.fillRect(px + 4, py + 14, 3, 2, armor);
    lcd.fillRect(px + 9, py + 14, 3, 2, armor);
    lcd.fillRect(px + 4, py + 16, 2, 6, suit);
    lcd.fillRect(px + 10, py + 16, 2, 6, suit);
    lcd.fillRect(px + 4, py + 22, 3, 2, boot);
    lcd.fillRect(px + 9, py + 22, 3, 2, boot);
    lcd.fillRect(px + 3, py + 18, 2, 3, pack);
    lcd.fillRect(px + 11, py + 18, 2, 3, pack);
    lcd.fillRect(px + 4, py + 5, 8, 2, visor);
    lcd.drawPixel(px + 5, py + 5, COLOR_WHITE);
    lcd.drawPixel(px + 10, py + 5, COLOR_WHITE);
    lcd.fillRect(px + 13, py + 10, 3, 2, COLOR_LIGHTGRAY);
}

void characterDrawCivilian(int px, int py) {
    const uint16_t skin = 0xFD33;
    const uint16_t hair = 0x49A0;
    const uint16_t coat = 0x738E;
    const uint16_t shirt = 0xFFE0;
    const uint16_t pants = 0x3186;
    const uint16_t shoe = 0x2104;

    lcd.fillRect(px + 4, py + 1, 8, 3, hair);
    lcd.fillRect(px + 3, py + 4, 10, 7, skin);
    lcd.fillRect(px + 4, py + 3, 8, 2, hair);
    lcd.drawPixel(px + 5, py + 7, COLOR_BLACK);
    lcd.drawPixel(px + 10, py + 7, COLOR_BLACK);
    lcd.drawPixel(px + 7, py + 9, 0xA800);
    lcd.fillRect(px + 4, py + 11, 8, 7, coat);
    lcd.fillRect(px + 3, py + 17, 10, 2, coat);
    lcd.fillRect(px + 5, py + 12, 6, 3, shirt);
    lcd.fillRect(px + 2, py + 12, 2, 6, skin);
    lcd.fillRect(px + 12, py + 12, 2, 6, skin);
    lcd.fillRect(px + 4, py + 19, 3, 6, pants);
    lcd.fillRect(px + 9, py + 19, 3, 6, pants);
    lcd.drawFastVLine(px + 7, py + 19, 6, COLOR_BLACK);
    lcd.fillRect(px + 3, py + 25, 4, 2, shoe);
    lcd.fillRect(px + 9, py + 25, 4, 2, shoe);
}

void characterDrawWizard(int px, int py) {
    const uint16_t robe = 0x500F;
    const uint16_t robeDark = 0x2808;
    const uint16_t trim = 0xA81F;
    const uint16_t aura = 0x680D;
    const uint16_t staff = 0xB596;
    const uint16_t boot = 0x2104;

    lcd.drawPixel(px + 2, py + 2, aura);
    lcd.drawPixel(px + 13, py + 2, aura);
    lcd.drawPixel(px + 1, py + 8, aura);
    lcd.drawPixel(px + 14, py + 8, aura);
    lcd.fillRect(px + 4, py + 1, 8, 3, robe);
    lcd.fillRect(px + 3, py + 4, 10, 10, robe);
    lcd.fillRect(px + 2, py + 14, 12, 3, robeDark);
    lcd.drawFastHLine(px + 3, py + 4, 10, trim);
    lcd.drawFastVLine(px + 5, py + 7, 8, robeDark);
    lcd.drawFastVLine(px + 10, py + 7, 8, robeDark);
    lcd.fillRect(px + 5, py + 5, 6, 5, COLOR_BLACK);
    lcd.drawPixel(px + 7, py + 6, COLOR_MAGENTA);
    lcd.drawPixel(px + 8, py + 6, COLOR_MAGENTA);
    lcd.fillRect(px + 4, py + 17, 2, 5, robeDark);
    lcd.fillRect(px + 10, py + 17, 2, 5, robeDark);
    lcd.fillRect(px + 4, py + 22, 3, 2, boot);
    lcd.fillRect(px + 9, py + 22, 3, 2, boot);
    lcd.drawFastVLine(px + 13, py + 3, 11, staff);
    lcd.fillRect(px + 12, py + 1, 3, 3, COLOR_CYAN);
    lcd.drawPixel(px + 13, py + 0, COLOR_WHITE);
}
