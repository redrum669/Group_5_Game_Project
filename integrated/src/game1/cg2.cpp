#include "game1/scenes/cg2.h"
#include "st7789.h"
#include "config.h"
#include "game1/entities/character.h"
#include "input.h"
#include "game1/entities/weapon.h"
#include "game1/systems/sound.h"
#include <Arduino.h>
#include <string.h>

static int cg2Phase = 0;
static uint32_t cg2PhaseTime = 0;
static uint32_t cg2MusicStartTime = 0;
static bool cg2Dirty = true;
static int textCharIdx = 0;
static uint32_t lastCharMs = 0;
static bool textDone = false;

static int ufoAnimX = -20;
static int ufoAnimY = 124;
static int ufoAnimState = 0;

static int commanderAnimX = 240;
static int cageAnimY = -60;
static bool isCageDropped = false;
static int warriorAnimX = 50;
static bool isPriestDead = false;
static int priestAnimX = 260;

struct Dialog {
    const char* speaker;
    const char* text;
    uint16_t color;
};

static Dialog dialogs[] = {
    {"[ EARTH WARRIOR ]", "Why are there so many altar\nartifacts here? And why aren't\nthey providing support for\ntheir soldiers?", COLOR_WHITE},
    {"[ ALIEN COMMANDER ]", "Hahaha, finally it worked!\nNow the strongest land warrior\nhas been born!", COLOR_WHITE},
    {"[ HIGH PRIEST ]", "All these sacrifices were\nworth it. To begin with, the\naliens had poor land combat\ncapabilities anyway. Now my\nGod! You will lead us towards\nthe future!", COLOR_WHITE},
    {"[ HIGH PRIEST ]", "No! Why treat me like this!\nI am the priest of the altar.\nWithout me, how could there be\nso many sentry soldiers here\nto serve you!", COLOR_WHITE},
    {"[ ALIEN COMMANDER ]", "You have always been just\na pawn.", COLOR_WHITE}
};

void cg2Init() {
    cg2Phase = 0;
    cg2Dirty = true;
    textCharIdx = 0;
    textDone = false;
    cg2PhaseTime = millis();
    ufoAnimX = -20;
    ufoAnimY = 124;
    ufoAnimState = 0;

    commanderAnimX = 240;
    cageAnimY = -60;
    isCageDropped = false;
    warriorAnimX = 50;
    isPriestDead = false;
    priestAnimX = 260;
}

static void drawAlien(int px, int py, int scale, bool flip = false) {
    int bx = px - 8 * scale;
    int by = py - 8 * scale;

    static const uint16_t layerSkin[16] = { // Alien face
        0x0000, 0x0000, 0x0000, 0x03C0, 0x03C0, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    };
    static const uint16_t layerEye[16] = { // Red eyes looking slightly left
        0x0000, 0x0000, 0x0000, 0x0180, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    };
    static const uint16_t layerArmor[16] = { // Main metallic armor
        0x03C0, 0x07E0, 0x07E0, 0x0420, 0x0420, 0x03C0, 0x03C0, 0x7FFE,
        0x7FFE, 0x6FF6, 0x27E4, 0x23C4, 0x07E0, 0x0660, 0x0660, 0x0660
    };
    static const uint16_t layerDark[16] = { // Shadows, under-armor
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0420, 0x0810, 0x0180, 0x0000, 0x0000
    };
    static const uint16_t layerNeon[16] = { // Cyan energy lines
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0180, 0x03C0, 0x0180, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    };

    uint16_t colSkin = 0x07E0; // Bright Green
    uint16_t colEye = 0xF800; // Bright Red
    uint16_t colArmor = 0x3186; // Dark Grey
    uint16_t colDark = 0x1082; // Very dark grey
    uint16_t colNeon = 0x07FF; // Cyan glowing core

    for (int row=0; row<16; row++) {
        uint16_t s = layerSkin[row];
        uint16_t e = layerEye[row];
        uint16_t a = layerArmor[row];
        uint16_t d = layerDark[row];
        uint16_t n = layerNeon[row];

        for (int col=0; col<16; col++) {
            uint16_t mask = (1 << (flip ? col : (15 - col)));
            uint16_t color = 0x0000;
            bool draw = false;

            if (e & mask) { color = colEye; draw = true; }
            else if (n & mask) { color = colNeon; draw = true; }
            else if (d & mask) { color = colDark; draw = true; }
            else if (s & mask) { color = colSkin; draw = true; }
            else if (a & mask) { color = colArmor; draw = true; }

            if (draw) {
                lcd.fillRect(bx + col*scale, by + row*scale, scale, scale, color);
            }
        }
    }
}
static void drawAlienTowerStatic(int ufoX, int ufoY) {
    // Tower Base
    lcd.fillRect(ufoX + 10, ufoY + 10, 60, 30, 0x3186); // Dark gray concrete
    for(int i=0; i<6; i++) lcd.drawFastVLine(ufoX + 15 + i*8, ufoY + 10, 30, 0x1082); // Ribs
    lcd.drawFastHLine(ufoX + 10, ufoY + 25, 60, 0x2104); // Base line

    // Mid section
    lcd.fillRect(ufoX + 15, ufoY - 20, 50, 30, 0x4208); // Mid section
    for(int i=0; i<5; i++) lcd.drawFastHLine(ufoX + 15, ufoY - 20 + i*6, 50, 0x2104); // Plating
    lcd.drawRect(ufoX + 20, ufoY - 15, 40, 20, 0x1082);

    // Upper tower
    lcd.fillRect(ufoX + 25, ufoY - 60, 30, 40, 0x528A); // Upper tower
    for(int i=0; i<4; i++) {
        lcd.drawFastHLine(ufoX + 25, ufoY - 60 + i*10, 30, 0x3186); // Rings
        if(i<3) lcd.drawFastVLine(ufoX + 28 + i*8, ufoY - 60, 40, 0x3186); // Vertical panels
    }

    // Neon glowing lines and energy cores
    lcd.drawFastVLine(ufoX + 39, ufoY - 50, 60, 0x07FF); // Cyan core energy
    lcd.drawFastVLine(ufoX + 40, ufoY - 50, 60, 0x03FF);

    // Tower Antenna & Spikes
    lcd.drawFastVLine(ufoX + 39, ufoY - 80, 20, 0xC618); // Gray antenna
    lcd.drawFastVLine(ufoX + 40, ufoY - 80, 20, 0xC618);

    // Tech details / Windows
    lcd.fillRect(ufoX + 20, ufoY - 10, 8, 15, 0x0000); // Dark window
    lcd.fillRect(ufoX + 52, ufoY - 10, 8, 15, 0x0000);
    lcd.drawFastHLine(ufoX + 20, ufoY - 5, 8, 0x07FF); // Cyan glow from window
    lcd.drawFastHLine(ufoX + 52, ufoY - 5, 8, 0xF800); // Red glow from window

    // Ramp / Entrance
    lcd.fillRect(ufoX + 25, ufoY + 30, 30, 10, 0x2104);
    // Doorway (pitch black)
    lcd.fillRect(ufoX + 30, ufoY + 10, 20, 20, 0x0000);
    // Door frame glowing
    lcd.drawRect(ufoX + 29, ufoY + 9, 22, 22, 0x07FF);
}

static void drawAlienTowerDynamic(int ufoX, int ufoY, uint32_t now) {
    // Blinking red tip
    if ((now / 200) % 2 == 0) {
        lcd.drawPixel(ufoX + 39, ufoY - 82, 0xF800);
        lcd.drawPixel(ufoX + 40, ufoY - 82, 0xF800);
    } else {
        lcd.drawPixel(ufoX + 39, ufoY - 82, 0x4208);
        lcd.drawPixel(ufoX + 40, ufoY - 82, 0x4208);
    }

    // Draw a spinning radar dish at the top
    int spinPhase = (now / 150) % 4;
    lcd.drawFastHLine(ufoX + 30, ufoY - 70, 20, 0x0000); // Clear old
    if (spinPhase == 0)      lcd.drawFastHLine(ufoX + 30, ufoY - 70, 20, 0xC618);
    else if (spinPhase == 1) lcd.drawFastHLine(ufoX + 35, ufoY - 70, 10, 0xC618);
    else if (spinPhase == 2) lcd.drawFastHLine(ufoX + 38, ufoY - 70, 4, 0xC618);
    else if (spinPhase == 3) lcd.drawFastHLine(ufoX + 35, ufoY - 70, 10, 0xC618);

    // Draw some floating energy orbs around tower
    for(int i=0; i<3; i++) {
        int orbX = ufoX + (i==0?-5:(i==1?85:39));
        // Clear old orb vertical path (only clear where the orb actually traverses)
        lcd.drawFastVLine(orbX, ufoY - 36, 12, 0x0000);
        lcd.drawFastVLine(orbX+1, ufoY - 36, 12, 0x0000);
        // Draw new orb using fast single-precision sinf
        int orbY = ufoY - 30 + (int)(sinf((now + i*500)/200.0f) * 5.0f);
        lcd.drawPixel(orbX, orbY, 0x07FF);
        lcd.drawPixel(orbX+1, orbY, 0x03FF);
    }
}

static void drawAlienGroundAndEntities(uint32_t now, bool isFullRedraw) {
    if (isFullRedraw) {
        // Alien bio-metallic floor instead of simple 0x1004
        lcd.fillRect(0, 140, 240, 100, 0x2104); // Darker grey-brown
        for (int i=0; i<240; i+=16) lcd.drawFastVLine(i, 140, 100, 0x1082); // Vertical grid
        for (int y=140; y<240; y+=16) lcd.drawFastHLine(0, y, 240, 0x1082); // Horizontal grid

        // Add an Alien Entity (Guard) standing next to the tower (outside char bounding box)
        drawAlien(190, 125, 2);
    }

    // Add some alien pods/structures on the ground (Safe at y >= 140)
    for(int i=0; i<4; i++) {
        int px = 10 + i*60;
        int py = 160 + (i%2)*15;
        if (isFullRedraw) {
            lcd.fillRect(px, py-15, 18, 20, 0x600C); // Purple bio-mass
            lcd.fillRect(px+3, py-20, 12, 5, 0x600C);
            lcd.drawRect(px+6, py-10, 6, 12, 0x07FF); // Cyan glowing vein
        }

        // Pulsing vein animation
        if ((now/300 + i) % 2 == 0) {
            lcd.fillRect(px+7, py-9, 4, 10, 0x07FF);
        } else {
            lcd.fillRect(px+7, py-9, 4, 10, 0x03FF);
        }
    }
}


static void drawArmoryBgRect(int rx, int ry, int rw, int rh) {
    if (rx < 0) { rw += rx; rx = 0; }
    if (ry < 0) { rh += ry; ry = 0; }
    if (rx + rw > 240) rw = 240 - rx;
    if (ry + rh > 140) rh = 140 - ry;
    if (rw <= 0 || rh <= 0) return;

    for (int y = ry; y < ry + rh; y++) {
        uint16_t bg = 0x2008;
        if (y > 115) bg = 0x1004;
        if (y == 115) bg = 0x600C;
        if (y < 35) bg = 0x1004;
        lcd.drawFastHLine(rx, y, rw, bg);
    }
}

static void drawWeaponRacks() {
    for (int i=0; i<6; i++) {
        int r_x = 10 + i*37;

        lcd.fillRect(r_x, 10, 32, 65, 0x18E3); // Gray metal plate
        lcd.drawRect(r_x, 10, 32, 65, 0x0000); // Black border
        lcd.drawRect(r_x + 2, 12, 28, 61, 0x03FF); // Inner cyan glow

        if (i == 0) {
            int rx1 = r_x;
            lcd.fillRect(rx1 + 10, 45, 14, 6, 0x8410);
            lcd.fillRect(rx1 + 7, 47, 3, 2, 0x4208);
            lcd.fillRect(rx1 + 24, 46, 6, 2, 0x4208);
            lcd.fillRect(rx1 + 12, 51, 4, 8, 0x2104);
            lcd.fillRect(rx1 + 18, 51, 3, 4, 0x2104);
        } else if (i == 1) {
            int rx2 = r_x;
            lcd.fillRect(rx2 + 14, 60, 4, 12, 0x632C);
            lcd.fillRect(rx2 + 12, 58, 8, 2, 0xCE59);
            lcd.fillRect(rx2 + 15, 18, 2, 40, 0x07E0);
            lcd.drawFastVLine(rx2 + 15, 18, 40, 0xFFFF);
        } else if (i == 2) {
            int rx3 = r_x;
            lcd.drawFastVLine(rx3 + 16, 15, 55, 0x4208);
            lcd.fillRect(rx3 + 11, 25, 10, 10, 0x8410);
            lcd.fillRect(rx3 + 13, 27, 6, 4, 0x07FF);
            lcd.fillRect(rx3 + 7, 40, 18, 14, 0x2104);
            lcd.drawFastHLine(rx3 + 5, 40, 22, 0xA514);
            lcd.fillRect(rx3 + 10, 41, 12, 10, 0x8410);
        } else if (i == 3) {
            int rx4 = r_x;
            lcd.fillRect(rx4 + 6, 40, 20, 10, 0xE71C);
            lcd.fillRect(rx4 + 4, 42, 4, 6, 0x4208);
            lcd.fillRect(rx4 + 26, 43, 10, 4, 0x07FF);
            lcd.drawFastHLine(rx4 + 26, 44, 10, 0xFFFF);
            lcd.fillRect(rx4 + 10, 50, 4, 5, 0x2104);
        } else if (i == 4) {
            int rx5 = r_x;
            lcd.fillRect(rx5 + 4, 38, 24, 12, 0x4208);
            lcd.fillRect(rx5 + 28, 42, 6, 4, 0x2104);
            lcd.fillRect(rx5 + 14, 50, 6, 12, 0x8410);
            lcd.fillRect(rx5 + 8, 50, 3, 5, 0x0000);
        } else if (i == 5) {
            int rx6 = r_x;
            lcd.drawFastVLine(rx6 + 16, 15, 55, 0x4208);
            lcd.fillRect(rx6 + 11, 25, 10, 10, 0x4208);
            lcd.fillRect(rx6 + 13, 27, 6, 4, 0xF800);
            lcd.fillRect(rx6 + 7, 40, 18, 14, 0x39A6);
            lcd.drawFastHLine(rx6 + 5, 40, 22, 0xA534);
            lcd.fillRect(rx6 + 10, 41, 12, 10, 0xA534);
        }
    }
}

void cg2Draw() {
    if (cg2Dirty) {
        if (cg2Phase == 23) {
            if (ufoAnimState == 0) {
                // Draw Transmission Log background
                lcd.fillScreen(COLOR_BLACK);

                // Large panel
                lcd.fillRect(10, 30, 220, 175, COLOR_BG_PANEL);
                lcd.drawRect(10, 30, 220, 175, COLOR_GOLD);
                lcd.drawRect(11, 31, 218, 173, 0x4208);   // inner shadow

                // Title line
                lcd.drawText(30, 42, "-- TRANSMISSION LOG --", COLOR_GOLD, COLOR_BG_PANEL, 1);
                lcd.drawFastHLine(12, 53, 216, COLOR_GOLD);
                lcd.drawFastHLine(12, 198, 216, COLOR_GOLD);
            }

            // Story text
            const char* lines[] = {
                "The last Sentinel Warrior",
                "fell to the ground.",
                "Upon his knees he bowed",
                "before the New Gods.",
                "Thus fell the Dawn Outpost",
                "of Mankind's defence line,",
                "broken and overrun.",
                "And so the age of Mankind's",
                "Star Warriors rose to reign."
            };
            uint16_t colors[] = {
                COLOR_WHITE, COLOR_WHITE, COLOR_LIGHTGRAY, COLOR_RED, COLOR_LIGHTGRAY,
                COLOR_LIGHTGRAY, COLOR_CYAN, COLOR_GOLD, COLOR_GOLD
            };
            int y_pos[] = {62, 76, 90, 110, 124, 138, 158, 172, 186};

            int charsDrawn = 0;
            for (int i = 0; i < 9; i++) {
                int len = strlen(lines[i]);
                if (ufoAnimState >= charsDrawn + len) {
                    lcd.drawText(18, y_pos[i], lines[i], colors[i], COLOR_BG_PANEL, 1);
                    charsDrawn += len;
                } else if (ufoAnimState > charsDrawn) {
                    int toDraw = ufoAnimState - charsDrawn;
                    char buf[32];
                    strncpy(buf, lines[i], toDraw);
                    buf[toDraw] = '\0';
                    lcd.drawText(18, y_pos[i], buf, colors[i], COLOR_BG_PANEL, 1);
                    charsDrawn += toDraw;
                    break;
                } else {
                    break;
                }
            }

            cg2Dirty = false;
            return;
        }

        if (cg2Phase == 30) {
            lcd.fillScreen(COLOR_BLACK);

            // Distant starry sky
            for (int i=0; i<40; i++) {
                int sx = (i * 37) % 240;
                int sy = (i * 19) % 90;
                lcd.drawPixel(sx, sy, (i % 3 == 0) ? 0x07FF : 0x7BEF);
            }

            // Distant alien mountains
            for (int mx=0; mx<240; mx++) {
                int mh = 15 + (mx * 7 % 15) + (mx * 3 % 8);
                lcd.drawFastVLine(mx, 90 - mh, mh, 0x1082);
            }

            // simple empty ground
            lcd.fillRect(0, 90, 240, 150, 0x18C3); // slightly lighter dark ground
            lcd.drawFastHLine(0, 90, 240, 0x2104);

            int size = ufoAnimState;
            if (size == 0) size = 2; // Initial state fallback

            g_playerCorrupted = true;
            characterSetDir(CHAR_DIR_DOWN);

            // Draw from back to front for proper overlap
            for (int r = size - 1; r >= 0; r--) {
                int scale;
                int y;
                int spacingX;

                // Perspective settings: front rows are larger and lower
                if (r < 2) {
                    scale = 3;
                    y = 190 - r * 18;
                    spacingX = 45;
                } else if (r < 5) {
                    scale = 2;
                    y = 150 - (r-2) * 12;
                    spacingX = 25;
                } else {
                    scale = 1;
                    y = 115 - (r-5) * 6;
                    spacingX = 14;
                }

                int cols = size;
                int startX = 120 - (cols - 1) * spacingX / 2;

                for (int c = 0; c < cols; c++) {
                    int x = startX + c * spacingX;
                    if (y > 80 && x > -20 && x < 260) { // Don't draw above horizon or far offscreen
                        characterDraw(x, y, 0, scale);
                    }
                }
            }

            cg2Dirty = false;
            return;
        }

        if (cg2Phase == 31) {
            lcd.fillScreen(COLOR_BLACK);

            lcd.drawText(36, 40, "The age of the", COLOR_WHITE, COLOR_BLACK, 2);
            lcd.drawText(36, 64, "Sentinels hath", COLOR_WHITE, COLOR_BLACK, 2);
            lcd.drawText(48, 88, "passed away;", COLOR_WHITE, COLOR_BLACK, 2);

            lcd.drawText(60, 136, "The era of", COLOR_WHITE, COLOR_BLACK, 2);
            lcd.drawText(42, 160, "Star Warriors", COLOR_WHITE, COLOR_BLACK, 2);
            lcd.drawText(18, 184, "is come to reign.", COLOR_WHITE, COLOR_BLACK, 2);

            if (textDone) {
                lcd.drawText(160, 218, "[A] Finish", COLOR_GRAY, COLOR_BLACK, 1);
            }
            cg2Dirty = false;
            return;
        }

        bool skipUpperRedraw = false;
        if (cg2Phase == 12 || cg2Phase == 14 || cg2Phase == 16 || cg2Phase == 18 || cg2Phase == 19 || cg2Phase == 21 || cg2Phase == 25) {
            skipUpperRedraw = true;
        }

        if (!skipUpperRedraw) {
            lcd.fillScreen(COLOR_BLACK);

            if (cg2Phase < 10) {
                int ufoX = 100;
                int ufoY = 100;

                drawAlienGroundAndEntities(0, true);

                // Background / Sky
                lcd.fillRect(0, 0, 240, 140, 0x0000);

                // Draw static stars
                for (int i=0; i<30; i++) {
                    int sx = (i * 37) % 240;
                    int sy = (i * 19) % 140;
                    lcd.drawPixel(sx, sy, (i % 3 == 0) ? 0x07FF : 0xFFFF);
                }

                // Distant mountains
                for (int mx=0; mx<240; mx++) {
                    int mh = 10 + (mx * 7 % 15) + (mx * 3 % 8);
                    lcd.drawFastVLine(mx, 140 - mh, mh, 0x1004);
                }

                drawAlienTowerStatic(ufoX, ufoY);
                drawAlienTowerDynamic(ufoX, ufoY, 0);

                if (ufoAnimState < 3) {
                    characterDraw(ufoAnimX, ufoAnimY, 0, 2); // 0 = WEAPON_SMG or default
                }

                cg2Dirty = false;
                return;
            }

            // Draw armory background and racks
            drawArmoryBgRect(0, 0, 240, 140);
            if (cg2Phase < 13) drawWeaponRacks();

            if (cg2Phase >= 10 && cg2Phase < 12) characterDraw(50, 105, 0, 3);
            else if (cg2Phase == 12) characterDraw(warriorAnimX, 105, 0, 3);
            else if (cg2Phase >= 13) characterDraw(50, 105, 0, 3);

            if (isCageDropped || cg2Phase >= 14) {
                int cx = 50 - 18, cy = cageAnimY - 28;
                if (cg2Phase >= 14 && cg2Phase < 18) cy = 105 - 28;
                lcd.fillRect(cx, cy, 36, 56, 0x0000);
                for (int i=0; i<6; i++) lcd.drawFastVLine(cx + 3 + i*6, cy, 56, 0xF800);
                lcd.drawRect(cx, cy, 36, 56, 0xFEA0);
                lcd.drawRect(cx-1, cy-1, 38, 58, 0xFEA0);
            }

            if (cg2Phase >= 13) {
                int ax = (cg2Phase == 13) ? commanderAnimX : 130;
                bool flip = (cg2Phase >= 18);
                drawAlien(ax, 105, 3, flip);
                // Commander details
                lcd.fillRect(ax - 6, 105 - 25, 12, 6, 0xFEA0); // Golden crown/helmet (smaller)
                lcd.fillRect(ax - 20, 105 - 10, 8, 10, 0xFEA0); // Left shoulder pad
                lcd.fillRect(ax + 12, 105 - 10, 8, 10, 0xFEA0); // Right shoulder pad
            }

            if (cg2Phase >= 14) {
                if (isPriestDead) {
                    characterDrawSkull(priestAnimX - 5, 105 + 5);
                } else {
                    if (priestAnimX < 240) {
                        characterDrawPriest(priestAnimX, 105);
                    }
                }
            }
        } else {
            // We just clear the bottom area for dialog rendering
            lcd.fillRect(0, 140, 240, 100, COLOR_BLACK);
        }

        int dialogIdx = -1;
        if (cg2Phase == 10 || cg2Phase == 11) dialogIdx = 0;
        else if (cg2Phase == 14 || cg2Phase == 15) dialogIdx = 1;
        else if (cg2Phase == 16 || cg2Phase == 17) dialogIdx = 2;
        else if (cg2Phase == 19 || cg2Phase == 20) dialogIdx = 3;
        else if (cg2Phase == 21 || cg2Phase == 22) dialogIdx = 4;

        if (dialogIdx != -1) {
            lcd.fillRect(10, 140, 220, 90, COLOR_BG_PANEL);
            lcd.drawRect(10, 140, 220, 90, COLOR_GOLD);
            lcd.drawRect(11, 141, 218, 88, 0x4208);
            lcd.drawText(15, 145, dialogs[dialogIdx].speaker, dialogs[dialogIdx].color, COLOR_BG_PANEL, 1);
            lcd.drawFastHLine(15, 155, 210, 0x4208);
        }

        cg2Dirty = false;
        textCharIdx = 0;
        textDone = false;
        lastCharMs = millis();
    }
}

static void drawLightningLine(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
        lcd.fillRect(x0, y0, 2, 2, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void cg2DrawUpdate() {
    uint32_t now = millis();

    if (cg2Phase < 10) {
        if (cg2Dirty) return;

        if (now - lastCharMs > 40) {
            lastCharMs = now;

            int ufoX = 100;
            int ufoY = 100;
            int oldX = ufoAnimX;
            int oldY = ufoAnimY;

            if (ufoAnimState == 0) {
                characterSetDir(CHAR_DIR_RIGHT);
                ufoAnimX += 3;
                if (ufoAnimX >= ufoX + 40) {
                    ufoAnimX = ufoX + 40;
                    ufoAnimState = 1;
                    cg2PhaseTime = now;
                }
            } else if (ufoAnimState == 1) {
                characterSetDir(CHAR_DIR_UP);
                if (now - cg2PhaseTime > 500) ufoAnimState = 2;
            } else if (ufoAnimState == 2) {
                characterSetDir(CHAR_DIR_UP);
                ufoAnimY -= 2;
                if (ufoAnimY <= ufoY + 20) {
                    ufoAnimState = 3;
                    cg2PhaseTime = now;
                }
            } else if (ufoAnimState == 3) {
                if (now - cg2PhaseTime > 1500) {
                    cg2Phase = 10;
                    cg2Dirty = true;
                }
                return;
            }

            // Erase old character bounding box
            lcd.fillRect(oldX - 16, oldY - 16, 32, 32, 0x0000);

            // Re-draw stars if they intersect
            for (int i=0; i<30; i++) {
                int sx = (i * 37) % 240;
                int sy = (i * 19) % 140;
                if (sx >= oldX - 16 && sx < oldX + 16 && sy >= oldY - 16 && sy < oldY + 16) {
                    lcd.drawPixel(sx, sy, (i % 3 == 0) ? 0x07FF : 0xFFFF);
                }
            }

            // Re-draw distant mountains if they intersect
            for (int mx = max(0, oldX - 16); mx < min(240, oldX + 16); mx++) {
                int mh = 10 + (mx * 7 % 15) + (mx * 3 % 8);
                if (140 - mh < oldY + 16 && 140 > oldY - 16) {
                    int drawY = max(140 - mh, oldY - 16);
                    int h = min(140, oldY + 16) - drawY;
                    if (h > 0) lcd.drawFastVLine(mx, drawY, h, 0x1004);
                }
            }

            // Re-draw ground (only where character overlaps, but they don't reach y > 140)
            if (oldY + 16 > 140) {
                int gy = 140;
                if (oldY - 16 > 140) gy = oldY - 16;
                lcd.fillRect(oldX - 16, gy, 32, (oldY + 16) - gy, 0x2104);
            }

            // Restore static tower parts IF they intersect with bounding box
            if (oldX + 16 > ufoX + 10 && oldX - 16 < ufoX + 70 && oldY + 16 > ufoY - 20) {
                // To prevent flickering of the whole tower, just redraw the Base and Mid Section where character walks
                // Mid section
                lcd.fillRect(ufoX + 15, ufoY - 20, 50, 30, 0x4208);
                for(int i=0; i<5; i++) lcd.drawFastHLine(ufoX + 15, ufoY - 20 + i*6, 50, 0x2104);
                lcd.drawRect(ufoX + 20, ufoY - 15, 40, 20, 0x1082);

                // Tower Base
                lcd.fillRect(ufoX + 10, ufoY + 10, 60, 30, 0x3186);
                for(int i=0; i<6; i++) lcd.drawFastVLine(ufoX + 15 + i*8, ufoY + 10, 30, 0x1082);
                lcd.drawFastHLine(ufoX + 10, ufoY + 25, 60, 0x2104);

                // Ramp / Entrance
                lcd.fillRect(ufoX + 25, ufoY + 30, 30, 10, 0x2104);
                lcd.fillRect(ufoX + 30, ufoY + 10, 20, 20, 0x0000);
                lcd.drawRect(ufoX + 29, ufoY + 9, 22, 22, 0x07FF);

                // Re-draw energy cores over the mid section we just erased
                lcd.drawFastVLine(ufoX + 39, ufoY - 20, 30, 0x07FF);
                lcd.drawFastVLine(ufoX + 40, ufoY - 20, 30, 0x03FF);
            }

            if (ufoAnimState < 3) {
                characterDraw(ufoAnimX, ufoAnimY, 0, 2);
            }

            drawAlienTowerDynamic(ufoX, ufoY, now);
            drawAlienGroundAndEntities(now, false);
        }
        return;
    }

    if (cg2Phase == 12) {
        if (cg2Dirty) return;

        if (now - lastCharMs > 40) {
            lastCharMs = now;

            int oldWarriorX = warriorAnimX;
            warriorAnimX += 6;
            if (warriorAnimX > 250) {
                cg2Phase = 13;
                cg2Dirty = true;
                return;
            }

            drawArmoryBgRect(oldWarriorX - 25, 105 - 28, 50, 60);
            characterDraw(warriorAnimX, 105, 0, 3);
        }
        return;
    }

    if (cg2Phase == 13) {
        if (cg2Dirty) return;

        if (now - lastCharMs > 40) {
            lastCharMs = now;

            int oldCmdX = commanderAnimX;
            int oldCageY = cageAnimY;
            bool animated = false;

            if (commanderAnimX > 130) {
                commanderAnimX -= 4;
                if (commanderAnimX < 130) commanderAnimX = 130;
                animated = true;
            } else if (!isCageDropped) {
                cageAnimY += 12;
                if (cageAnimY >= 105) {
                    cageAnimY = 105;
                    isCageDropped = true;
                    cg2PhaseTime = now;
                }
                animated = true;
            } else {
                if (now - cg2PhaseTime > 500) {
                    cg2Phase = 14;
                    cg2Dirty = true;
                }
            }

            if (animated) {
                if (oldCageY != cageAnimY) {
                    drawArmoryBgRect(50 - 20, oldCageY - 30, 42, 62);
                }
                if (oldCmdX != commanderAnimX) {
                    drawArmoryBgRect(oldCmdX - 25, 105 - 26, 52, 52);
                }

                characterDraw(50, 105, 0, 3);

                int cx = 50 - 18, cy = cageAnimY - 28;
                lcd.fillRect(cx, cy, 36, 56, 0x0000);
                for (int i=0; i<6; i++) lcd.drawFastVLine(cx + 3 + i*6, cy, 56, 0xF800);
                lcd.drawRect(cx, cy, 36, 56, 0xFEA0);
                lcd.drawRect(cx-1, cy-1, 38, 58, 0xFEA0);

                drawAlien(commanderAnimX, 105, 3);
                lcd.fillRect(commanderAnimX - 6, 105 - 25, 12, 6, 0xFEA0);
                lcd.fillRect(commanderAnimX - 20, 105 - 10, 8, 10, 0xFEA0);
                lcd.fillRect(commanderAnimX + 12, 105 - 10, 8, 10, 0xFEA0);
            }
        }
        return;
    }

    if (cg2Phase == 25) {
        if (cg2Dirty) return;

        if (now - lastCharMs > 40) {
            lastCharMs = now;

            int oldPriestX = priestAnimX;
            bool animated = false;

            if (priestAnimX > 190) {
                priestAnimX -= 4;
                if (priestAnimX < 190) priestAnimX = 190;
                animated = true;
            } else {
                if (now - cg2PhaseTime > 300) {
                    cg2Phase = 16;
                    cg2Dirty = true;
                }
            }

            if (animated) {
                if (oldPriestX != priestAnimX) {
                    drawArmoryBgRect(oldPriestX - 5, 95, 30, 45);
                }
                if (priestAnimX < 240) {
                    characterDrawPriest(priestAnimX, 105);
                }
            }
        }
        return;
    }

    if (cg2Phase == 18) {
        if (cg2Dirty) return;

        if (now - lastCharMs > 40) {
            lastCharMs = now;
            int oldCageY = cageAnimY;
            bool animated = false;

            if (cageAnimY > -60) {
                cageAnimY -= 12;
                if (cageAnimY < -60) cageAnimY = -60;
                animated = true;
            }

            if (animated) {
                if (oldCageY != cageAnimY) {
                    drawArmoryBgRect(50 - 20, oldCageY - 30, 42, 62);
                }
                characterDraw(50, 105, 0, 3);

                int cx = 50 - 18, cy = cageAnimY - 28;
                lcd.fillRect(cx, cy, 36, 56, 0x0000);
                for (int i=0; i<6; i++) lcd.drawFastVLine(cx + 3 + i*6, cy, 56, 0xF800);
                lcd.drawRect(cx, cy, 36, 56, 0xFEA0);
                lcd.drawRect(cx-1, cy-1, 38, 58, 0xFEA0);
            } else {
                uint32_t phaseElapsed = now - cg2PhaseTime;
                if (phaseElapsed > 500 && phaseElapsed < 1200) { // Extended duration for lightning
                    // Draw lightning to the High Priest (flashing effect)
                    if ((phaseElapsed / 50) % 2 == 0) { // Flash every 50ms
                        drawLightningLine(commanderAnimX, 105 - 20, 190, 105, COLOR_WHITE);
                        drawLightningLine(commanderAnimX, 105 - 20, 190 + 5, 105 - 5, COLOR_CYAN);

                        // Add some randomized smaller bolts for extra effect
                        drawLightningLine(commanderAnimX + (millis() % 10 - 5), 105 - 20 + (millis() % 10 - 5), 190 + (millis() % 20 - 10), 105 + (millis() % 20 - 10), COLOR_WHITE);
                    } else {
                        // Clear the lightning slightly by redrawing background over it to create flash
                        drawArmoryBgRect(commanderAnimX, 105 - 30, 190 - commanderAnimX + 25, 60);
                        // Redraw entities that might have been cleared
                        drawAlien(commanderAnimX, 105, 3, false);
                        lcd.fillRect(commanderAnimX - 6, 105 - 25, 12, 6, 0xFEA0); // smaller crown
                        lcd.fillRect(commanderAnimX - 20, 105 - 10, 8, 10, 0xFEA0);
                        lcd.fillRect(commanderAnimX + 12, 105 - 10, 8, 10, 0xFEA0);
                        // Make priest shake/jitter during electrocution
                        characterDraw(190 + (millis() % 6 - 3), 105 + (millis() % 4 - 2), 2, 3);
                    }
                } else if (phaseElapsed >= 1200 && phaseElapsed < 1400) {
                    // A massive final flash! White out the area where the priest is
                    drawArmoryBgRect(commanderAnimX, 105 - 30, 190 - commanderAnimX + 25, 60);
                    drawAlien(commanderAnimX, 105, 3, false);
                    lcd.fillRect(commanderAnimX - 6, 105 - 25, 12, 6, 0xFEA0); // smaller crown
                    lcd.fillRect(commanderAnimX - 20, 105 - 10, 8, 10, 0xFEA0);
                    lcd.fillRect(commanderAnimX + 12, 105 - 10, 8, 10, 0xFEA0);

                    // Final thick lightning bolt
                    drawLightningLine(commanderAnimX, 105 - 20, 190, 105, COLOR_WHITE);
                    drawLightningLine(commanderAnimX, 105 - 20, 190 + 2, 105 - 2, COLOR_WHITE);
                    drawLightningLine(commanderAnimX, 105 - 20, 190 - 2, 105 + 2, COLOR_WHITE);
                    drawLightningLine(commanderAnimX, 105 - 20, 190 + 5, 105 - 5, COLOR_CYAN);
                    drawLightningLine(commanderAnimX, 105 - 20, 190 - 5, 105 + 5, COLOR_CYAN);

                    // The explosion at the priest
                    lcd.fillRect(190 - 15, 105 - 15, 30, 30, COLOR_WHITE);
                    lcd.fillRect(190 - 10, 105 - 10, 20, 20, COLOR_CYAN);
                } else if (phaseElapsed >= 1400 && phaseElapsed < 2500) { // Extended delay before moving to next phase
                    if (!isPriestDead) {
                        isPriestDead = true;
                        drawArmoryBgRect(commanderAnimX, 105 - 30, 190 - commanderAnimX + 25, 60); // Clear priest and lightning path
                        drawArmoryBgRect(190 - 20, 105 - 20, 40, 40); // clear explosion
                        drawAlien(commanderAnimX, 105, 3, true);
                        lcd.fillRect(commanderAnimX - 6, 105 - 25, 12, 6, 0xFEA0); // smaller crown
                        lcd.fillRect(commanderAnimX - 20, 105 - 10, 8, 10, 0xFEA0);
                        lcd.fillRect(commanderAnimX + 12, 105 - 10, 8, 10, 0xFEA0);
                        characterDrawSkull(190 - 5, 105 + 5);
                    }
                } else if (phaseElapsed >= 2500) {
                    cg2Phase = 19;
                    cg2Dirty = true;
                }
            }
        }
        return;
    }

    if (cg2Phase == 19 || cg2Phase == 20) {
        if (cg2Dirty) return;

        int dialogIdx = 3;
        const char* text = dialogs[dialogIdx].text;
        int len = strlen(text);

        if (now - lastCharMs > 30) {
            lastCharMs = now;
            if (textCharIdx < len) {
                int line = 0, col = 0;
                for (int i=0; i<textCharIdx; i++) {
                    if (text[i] == '\n') { line++; col=0; }
                    else col++;
                }
                if (text[textCharIdx] != '\n') {
                    lcd.drawChar5x7(15 + col * 6, 160 + line * 10, text[textCharIdx], dialogs[dialogIdx].color, COLOR_BG_PANEL, 1);
                }
                textCharIdx++;
            } else {
                textDone = true;
                lcd.drawText(160, 218, "[A] Next", COLOR_GRAY, COLOR_BG_PANEL, 1);
            }
        }
        return;
    }

    if (cg2Phase == 23) {
        uint32_t elapsed = now - cg2PhaseTime;
        if (elapsed >= 16150) { // 14.94s + 1.21s = 16.15s total for log screen
            cg2Phase = 30;
            ufoAnimState = 1;
            cg2Dirty = true;
            textDone = false;
        } else {
            // Text finishes typing at exactly 14.94s, then stays on screen for 1.21s
            int currentChars = (elapsed * 214) / 14940;
            if (currentChars > 213) currentChars = 213;
            if (currentChars != ufoAnimState) {
                ufoAnimState = currentChars;
                cg2Dirty = true;
            }
        }
        return;
    }


    if (cg2Phase == 30) {
        if (cg2Dirty) return;
        uint32_t elapsed = now - cg2PhaseTime;
        int size = 1;
        if (elapsed >= 29900) size = 14;
        else if (elapsed >= 28000) size = 12;
        else if (elapsed >= 26170) size = 10;
        else if (elapsed >= 24340) size = 8;
        else if (elapsed >= 22510) size = 6;
        else if (elapsed >= 20620) size = 4;
        else if (elapsed >= 18820) size = 2;
        else size = 1;

        if (size != ufoAnimState) {
            ufoAnimState = size;
            cg2Dirty = true;
        }

        if (elapsed > 31900) {
            cg2Phase = 31;
            cg2PhaseTime = now;
            cg2Dirty = true;
            textDone = false;
        }
        return;
    }

    if (cg2Phase == 31) {
        if (cg2Dirty) return;
        uint32_t elapsed = now - cg2PhaseTime;
        if (elapsed > 2000 && !textDone) {
            textDone = true;
            cg2Dirty = true;
        }
        return;
    }

    if (cg2Dirty) return;
    if (textDone) return;

    int dialogIdx = -1;
    if (cg2Phase == 10 || cg2Phase == 11) dialogIdx = 0;
    else if (cg2Phase == 14 || cg2Phase == 15) dialogIdx = 1;
    else if (cg2Phase == 16 || cg2Phase == 17) dialogIdx = 2;
    else if (cg2Phase == 19 || cg2Phase == 20) dialogIdx = 3;
    else if (cg2Phase == 21 || cg2Phase == 22) dialogIdx = 4;

    if (dialogIdx == -1) {
        textDone = true;
        return;
    }

    const char* text = dialogs[dialogIdx].text;
    int len = strlen(text);

    if (now - lastCharMs > 30) {
        lastCharMs = now;

        if (textCharIdx < len) {
            int line = 0, col = 0;
            for (int i=0; i<textCharIdx; i++) {
                if (text[i] == '\n') { line++; col=0; }
                else col++;
            }
            if (text[textCharIdx] != '\n') {
                lcd.drawChar5x7(15 + col * 6, 160 + line * 10, text[textCharIdx], dialogs[dialogIdx].color, COLOR_BG_PANEL, 1);
            }
            textCharIdx++;
        } else {
            textDone = true;
            lcd.drawText(160, 218, "[A] Next", COLOR_GRAY, COLOR_BG_PANEL, 1);
        }
    }
}

bool cg2Update() {
    static bool prevBtnA = false;
    static uint32_t lastPressMs = 0;

    bool curBtnA = (digitalRead(BTN_A) == LOW);
    bool pressed = curBtnA && !prevBtnA;
    prevBtnA = curBtnA;

    if (pressed) {
        if (millis() - lastPressMs < 300) {
            pressed = false;
        } else {
            lastPressMs = millis();
        }
    }

    if (pressed) {
        if (cg2Phase < 10) {
            // Skip UFO animation
            cg2Phase = 10;
            cg2Dirty = true;
            return false;
        }

        if (!textDone) {
            int dialogIdx = -1;
            if (cg2Phase == 10 || cg2Phase == 11) dialogIdx = 0;
            else if (cg2Phase == 14 || cg2Phase == 15) dialogIdx = 1;
            else if (cg2Phase == 16 || cg2Phase == 17) dialogIdx = 2;
            else if (cg2Phase == 19 || cg2Phase == 20) dialogIdx = 3;
            else if (cg2Phase == 21 || cg2Phase == 22) dialogIdx = 4;

            if (dialogIdx != -1) {
                const char* text = dialogs[dialogIdx].text;
                int len = strlen(text);
                while (textCharIdx < len) {
                    int line = 0, col = 0;
                    for (int i=0; i<textCharIdx; i++) {
                        if (text[i] == '\n') { line++; col=0; }
                        else col++;
                    }
                    if (text[textCharIdx] != '\n') {
                        lcd.drawChar5x7(15 + col * 6, 160 + line * 10, text[textCharIdx], dialogs[dialogIdx].color, COLOR_BG_PANEL, 1);
                    }
                    textCharIdx++;
                }
                textDone = true;
                lcd.drawText(160, 218, "[A] Next", COLOR_GRAY, COLOR_BG_PANEL, 1);
            }
        } else {
            if (cg2Phase == 10 || cg2Phase == 11) {
                cg2Phase = 12;
            }
            else if (cg2Phase == 14 || cg2Phase == 15) {
                cg2Phase = 25;
                cg2PhaseTime = millis();
            }
            else if (cg2Phase == 16 || cg2Phase == 17) {
                cg2Phase = 18;
                cg2PhaseTime = millis();
                g_playerCorrupted = true;
            }
            else if (cg2Phase == 19 || cg2Phase == 20) {
                cg2Phase = 21;
                textDone = false;
                textCharIdx = 0;
            }
            else if (cg2Phase == 21 || cg2Phase == 22) {
                cg2Phase = 23;
                textDone = false;
                playTransmissionLogMusic();
                cg2PhaseTime = millis();
                ufoAnimState = 0;
            }
            else if (cg2Phase == 31 && textDone) {
                // stopTransmissionLogMusic(); // let it play uninterrupted
                return true;
            }
            cg2Dirty = true;
        }
    }
    return false;
}
