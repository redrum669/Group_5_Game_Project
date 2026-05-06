#include "cg2.h"
#include "st7789.h"
#include "config.h"
#include "character.h"
#include "input.h"
#include "weapon.h"
#include "sound.h"
#include <Arduino.h>
#include <string.h>
#include <math.h>

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

static void drawAlien(int px, int py, int scale, bool flip) {
    int bx = px - 8 * scale;
    int by = py - 8 * scale;

    static const uint16_t layerSkin[16] = {
        0x0000, 0x0000, 0x0000, 0x03C0, 0x03C0, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    };
    static const uint16_t layerEye[16] = {
        0x0000, 0x0000, 0x0000, 0x0180, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    };
    static const uint16_t layerArmor[16] = {
        0x03C0, 0x07E0, 0x07E0, 0x0420, 0x0420, 0x03C0, 0x03C0, 0x7FFE,
        0x7FFE, 0x6FF6, 0x27E4, 0x23C4, 0x07E0, 0x0660, 0x0660, 0x0660
    };
    static const uint16_t layerDark[16] = {
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0420, 0x0810, 0x0180, 0x0000, 0x0000
    };
    static const uint16_t layerNeon[16] = {
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0180, 0x03C0, 0x0180, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    };

    uint16_t colSkin = 0x07E0;
    uint16_t colEye = 0xF800;
    uint16_t colArmor = 0x3186;
    uint16_t colDark = 0x1082;
    uint16_t colNeon = 0x07FF;

    for (int row = 0; row < 16; row++) {
        uint16_t s = layerSkin[row];
        uint16_t e = layerEye[row];
        uint16_t a = layerArmor[row];
        uint16_t d = layerDark[row];
        uint16_t n = layerNeon[row];

        for (int col = 0; col < 16; col++) {
            uint16_t mask = 1 << (flip ? col : (15 - col));
            uint16_t color = 0x0000;
            bool draw = false;

            if (e & mask) { color = colEye; draw = true; }
            else if (n & mask) { color = colNeon; draw = true; }
            else if (d & mask) { color = colDark; draw = true; }
            else if (s & mask) { color = colSkin; draw = true; }
            else if (a & mask) { color = colArmor; draw = true; }

            if (draw) {
                lcd.fillRect(bx + col * scale, by + row * scale, scale, scale, color);
            }
        }
    }
}

static void drawAlienTowerStatic(int ufoX, int ufoY) {
    lcd.fillRect(ufoX + 10, ufoY + 10, 60, 30, 0x3186);
    for (int i = 0; i < 6; i++)
        lcd.drawFastVLine(ufoX + 15 + i * 8, ufoY + 10, 30, 0x1082);
    lcd.drawFastHLine(ufoX + 10, ufoY + 25, 60, 0x2104);

    lcd.fillRect(ufoX + 15, ufoY - 20, 50, 30, 0x4208);
    for (int i = 0; i < 5; i++)
        lcd.drawFastHLine(ufoX + 15, ufoY - 20 + i * 6, 50, 0x2104);
    lcd.drawRect(ufoX + 20, ufoY - 15, 40, 20, 0x1082);

    lcd.fillRect(ufoX + 25, ufoY - 60, 30, 40, 0x528A);
    for (int i = 0; i < 4; i++) {
        lcd.drawFastHLine(ufoX + 25, ufoY - 60 + i * 10, 30, 0x3186);
        if (i < 3)
            lcd.drawFastVLine(ufoX + 28 + i * 8, ufoY - 60, 40, 0x3186);
    }

    lcd.drawFastVLine(ufoX + 39, ufoY - 50, 60, 0x07FF);
    lcd.drawFastVLine(ufoX + 40, ufoY - 50, 60, 0x03FF);

    lcd.drawFastVLine(ufoX + 39, ufoY - 80, 20, 0xC618);
    lcd.drawFastVLine(ufoX + 40, ufoY - 80, 20, 0xC618);

    lcd.fillRect(ufoX + 20, ufoY - 10, 8, 15, 0x0000);
    lcd.fillRect(ufoX + 52, ufoY - 10, 8, 15, 0x0000);
    lcd.drawFastHLine(ufoX + 20, ufoY - 5, 8, 0x07FF);
    lcd.drawFastHLine(ufoX + 52, ufoY - 5, 8, 0xF800);

    lcd.fillRect(ufoX + 25, ufoY + 30, 30, 10, 0x2104);
    lcd.fillRect(ufoX + 30, ufoY + 10, 20, 20, 0x0000);
    lcd.drawRect(ufoX + 29, ufoY + 9, 22, 22, 0x07FF);
}

static void drawAlienTowerDynamic(int ufoX, int ufoY, uint32_t now) {
    if ((now / 200) % 2 == 0) {
        lcd.drawPixel(ufoX + 39, ufoY - 82, 0xF800);
        lcd.drawPixel(ufoX + 40, ufoY - 82, 0xF800);
    } else {
        lcd.drawPixel(ufoX + 39, ufoY - 82, 0x4208);
        lcd.drawPixel(ufoX + 40, ufoY - 82, 0x4208);
    }

    int spinPhase = (now / 150) % 4;
    lcd.drawFastHLine(ufoX + 30, ufoY - 70, 20, 0x0000);
    if (spinPhase == 0)
        lcd.drawFastHLine(ufoX + 30, ufoY - 70, 20, 0xC618);
    else if (spinPhase == 1)
        lcd.drawFastHLine(ufoX + 35, ufoY - 70, 10, 0xC618);
    else if (spinPhase == 2)
        lcd.drawFastHLine(ufoX + 38, ufoY - 70, 4, 0xC618);
    else if (spinPhase == 3)
        lcd.drawFastHLine(ufoX + 35, ufoY - 70, 10, 0xC618);

    for (int i = 0; i < 3; i++) {
        int orbX = ufoX + (i == 0 ? -5 : (i == 1 ? 85 : 39));
        lcd.drawFastVLine(orbX, ufoY - 36, 12, 0x0000);
        lcd.drawFastVLine(orbX + 1, ufoY - 36, 12, 0x0000);
        int orbY = ufoY - 30 + (int)(sinf((now + i * 500) / 200.0f) * 5.0f);
        lcd.drawPixel(orbX, orbY, 0x07FF);
        lcd.drawPixel(orbX + 1, orbY, 0x03FF);
    }
}

static void drawAlienGroundAndEntities(uint32_t now, bool isFullRedraw) {
    if (isFullRedraw) {
        lcd.fillRect(0, 140, 240, 100, 0x2104);
        for (int i = 0; i < 240; i += 16)
            lcd.drawFastVLine(i, 140, 100, 0x1082);
        for (int y = 140; y < 240; y += 16)
            lcd.drawFastHLine(0, y, 240, 0x1082);
        drawAlien(190, 125, 2);
    }

    for (int i = 0; i < 4; i++) {
        int px = 10 + i * 60;
        int py = 160 + (i % 2) * 15;
        if (isFullRedraw) {
            lcd.fillRect(px, py - 15, 18, 20, 0x600C);
            lcd.fillRect(px + 3, py - 20, 12, 5, 0x600C);
            lcd.drawRect(px + 6, py - 10, 6, 12, 0x07FF);
        }
        if ((now / 300 + i) % 2 == 0)
            lcd.fillRect(px + 7, py - 9, 4, 10, 0x07FF);
        else
            lcd.fillRect(px + 7, py - 9, 4, 10, 0x03FF);
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
    for (int i = 0; i < 6; i++) {
        int r_x = 10 + i * 37;
        lcd.fillRect(r_x, 10, 32, 65, 0x18E3);
        lcd.drawRect(r_x, 10, 32, 65, 0x0000);
        lcd.drawRect(r_x + 2, 12, 28, 61, 0x03FF);

        if (i == 0) {
            lcd.fillRect(r_x + 10, 45, 14, 6, 0x8410);
            lcd.fillRect(r_x + 7, 47, 3, 2, 0x4208);
            lcd.fillRect(r_x + 24, 46, 6, 2, 0x4208);
            lcd.fillRect(r_x + 12, 51, 4, 8, 0x2104);
            lcd.fillRect(r_x + 18, 51, 3, 4, 0x2104);
        } else if (i == 1) {
            lcd.fillRect(r_x + 14, 60, 4, 12, 0x632C);
            lcd.fillRect(r_x + 12, 58, 8, 2, 0xCE59);
            lcd.fillRect(r_x + 15, 18, 2, 40, 0x07E0);
            lcd.drawFastVLine(r_x + 15, 18, 40, 0xFFFF);
        } else if (i == 2) {
            lcd.drawFastVLine(r_x + 16, 15, 55, 0x4208);
            lcd.fillRect(r_x + 11, 25, 10, 10, 0x8410);
            lcd.fillRect(r_x + 13, 27, 6, 4, 0x07FF);
            lcd.fillRect(r_x + 7, 40, 18, 14, 0x2104);
            lcd.drawFastHLine(r_x + 5, 40, 22, 0xA514);
            lcd.fillRect(r_x + 10, 41, 12, 10, 0x8410);
        } else if (i == 3) {
            lcd.fillRect(r_x + 6, 40, 20, 10, 0xE71C);
            lcd.fillRect(r_x + 4, 42, 4, 6, 0x4208);
            lcd.fillRect(r_x + 26, 43, 10, 4, 0x07FF);
            lcd.drawFastHLine(r_x + 26, 44, 10, 0xFFFF);
            lcd.fillRect(r_x + 10, 50, 4, 5, 0x2104);
        } else if (i == 4) {
            lcd.fillRect(r_x + 4, 38, 24, 12, 0x4208);
            lcd.fillRect(r_x + 28, 42, 6, 4, 0x2104);
            lcd.fillRect(r_x + 14, 50, 6, 12, 0x8410);
            lcd.fillRect(r_x + 8, 50, 3, 5, 0x0000);
        } else if (i == 5) {
            lcd.drawFastVLine(r_x + 16, 15, 55, 0x4208);
            lcd.fillRect(r_x + 11, 25, 10, 10, 0x4208);
            lcd.fillRect(r_x + 13, 27, 6, 4, 0xF800);
            lcd.fillRect(r_x + 7, 40, 18, 14, 0x39A6);
            lcd.drawFastHLine(r_x + 5, 40, 22, 0xA534);
            lcd.fillRect(r_x + 10, 41, 12, 10, 0xA534);
        }
    }
}

void cg2Draw() {
    if (cg2Dirty) {
        if (cg2Phase == 23) {
            if (ufoAnimState == 0) {
                lcd.fillScreen(COLOR_BLACK);
                lcd.fillRect(10, 30, 220, 175, COLOR_BG_PANEL);
                lcd.drawRect(10, 30, 220, 175, COLOR_GOLD);
                lcd.drawRect(11, 31, 218, 173, 0x4208);
                lcd.drawText(30, 42, "-- TRANSMISSION LOG --", COLOR_GOLD, COLOR_BG_PANEL, 1);
                lcd.drawFastHLine(12, 53, 216, COLOR_GOLD);
                lcd.drawFastHLine(12, 198, 216, COLOR_GOLD);
            }

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
                    break;
                } else break;
            }
            cg2Dirty = false;
            return;
        }

        if (cg2Phase == 30) {
            lcd.fillScreen(COLOR_BLACK);
            for (int i = 0; i < 40; i++) {
                int sx = (i * 37) % 240;
                int sy = (i * 19) % 90;
                lcd.drawPixel(sx, sy, (i % 3 == 0) ? 0x07FF : 0x7BEF);
            }
            for (int mx = 0; mx < 240; mx++) {
                int mh = 15 + (mx * 7 % 15) + (mx * 3 % 8);
                lcd.drawFastVLine(mx, 90 - mh, mh, 0x1082);
            }
            lcd.fillRect(0, 90, 240, 150, 0x18C3);
            lcd.drawFastHLine(0, 90, 240, 0x2104);

            int size = ufoAnimState < 2 ? 2 : ufoAnimState;
            g_playerCorrupted = true;
            characterSetDir(CHAR_DIR_DOWN);

            for (int r = size - 1; r >= 0; r--) {
                int scale, y, spacingX;
                if (r < 2) { scale = 3; y = 190 - r * 18; spacingX = 45; }
                else if (r < 5) { scale = 2; y = 150 - (r - 2) * 12; spacingX = 25; }
                else { scale = 1; y = 115 - (r - 5) * 6; spacingX = 14; }

                int cols = size;
                int startX = 120 - (cols - 1) * spacingX / 2;
                for (int c = 0; c < cols; c++) {
                    int x = startX + c * spacingX;
                    if (y > 80 && x > -20 && x < 260)
                        characterDraw(x, y, 0, scale);
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
            if (textDone) lcd.drawText(160, 218, "[A] Finish", COLOR_GRAY, COLOR_BLACK, 1);
            cg2Dirty = false;
            return;
        }

        bool skipUpper = (cg2Phase == 12 || cg2Phase == 14 || cg2Phase == 16 ||
                          cg2Phase == 18 || cg2Phase == 19 || cg2Phase == 21 || cg2Phase == 25);
        if (!skipUpper) {
            lcd.fillScreen(COLOR_BLACK);
            if (cg2Phase < 10) {
                int ufoX = 100, ufoY = 100;
                drawAlienGroundAndEntities(0, true);
                lcd.fillRect(0, 0, 240, 140, COLOR_BLACK);
                for (int i = 0; i < 30; i++) {
                    int sx = (i * 37) % 240;
                    int sy = (i * 19) % 140;
                    lcd.drawPixel(sx, sy, (i % 3 == 0) ? 0x07FF : COLOR_WHITE);
                }
                for (int mx = 0; mx < 240; mx++) {
                    int mh = 10 + (mx * 7 % 15) + (mx * 3 % 8);
                    lcd.drawFastVLine(mx, 140 - mh, mh, 0x1004);
                }
                drawAlienTowerStatic(ufoX, ufoY);
                drawAlienTowerDynamic(ufoX, ufoY, 0);
                if (ufoAnimState < 3) characterDraw(ufoAnimX, ufoAnimY, 0, 2);
                cg2Dirty = false;
                return;
            }

            drawArmoryBgRect(0, 0, 240, 140);
            if (cg2Phase < 13) drawWeaponRacks();

            if (cg2Phase >= 10 && cg2Phase < 12) characterDraw(50, 105, 0, 3);
            else if (cg2Phase == 12) characterDraw(warriorAnimX, 105, 0, 3);
            else if (cg2Phase >= 13) characterDraw(50, 105, 0, 3);

            if (isCageDropped || cg2Phase >= 14) {
                int cx = 50 - 18, cy = cageAnimY - 28;
                if (cg2Phase >= 14 && cg2Phase < 18) cy = 105 - 28;
                lcd.fillRect(cx, cy, 36, 56, COLOR_BLACK);
                for (int i = 0; i < 6; i++)
                    lcd.drawFastVLine(cx + 3 + i * 6, cy, 56, COLOR_RED);
                lcd.drawRect(cx, cy, 36, 56, COLOR_YELLOW);
                lcd.drawRect(cx - 1, cy - 1, 38, 58, COLOR_YELLOW);
            }

            if (cg2Phase >= 13) {
                int ax = (cg2Phase == 13) ? commanderAnimX : 130;
                bool flip = (cg2Phase >= 18);
                drawAlien(ax, 105, 3, flip);
                lcd.fillRect(ax - 6, 105 - 25, 12, 6, COLOR_YELLOW);
                lcd.fillRect(ax - 20, 105 - 10, 8, 10, COLOR_YELLOW);
                lcd.fillRect(ax + 12, 105 - 10, 8, 10, COLOR_YELLOW);
            }

            if (cg2Phase >= 14) {
                if (isPriestDead) characterDrawSkull(priestAnimX - 5, 105 + 5);
                else if (priestAnimX < 240) characterDrawPriest(priestAnimX, 105);
            }
        } else {
            lcd.fillRect(0, 140, 240, 100, COLOR_BLACK);
        }

        int dlg = -1;
        if (cg2Phase == 10 || cg2Phase == 11) dlg = 0;
        else if (cg2Phase == 14 || cg2Phase == 15) dlg = 1;
        else if (cg2Phase == 16 || cg2Phase == 17) dlg = 2;
        else if (cg2Phase == 19 || cg2Phase == 20) dlg = 3;
        else if (cg2Phase == 21 || cg2Phase == 22) dlg = 4;

        if (dlg != -1) {
            lcd.fillRect(10, 140, 220, 90, COLOR_BG_PANEL);
            lcd.drawRect(10, 140, 220, 90, COLOR_GOLD);
            lcd.drawRect(11, 141, 218, 88, 0x4208);
            lcd.drawText(15, 145, dialogs[dlg].speaker, dialogs[dlg].color, COLOR_BG_PANEL, 1);
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
        if (cg2Dirty || now - lastCharMs < 40) return;
        lastCharMs = now;

        int ufoX = 100, ufoY = 100;
        int oldX = ufoAnimX, oldY = ufoAnimY;

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
            if (ufoAnimY <= ufoY + 20) ufoAnimState = 3;
        } else if (ufoAnimState == 3 && now - cg2PhaseTime > 1500) {
            cg2Phase = 10;
            cg2Dirty = true;
            return;
        }

        lcd.fillRect(oldX - 16, oldY - 16, 32, 32, COLOR_BLACK);
        for (int i = 0; i < 30; i++) {
            int sx = (i * 37) % 240, sy = (i * 19) % 140;
            if (sx >= oldX - 16 && sx < oldX + 16 && sy >= oldY - 16 && sy < oldY + 16)
                lcd.drawPixel(sx, sy, (i % 3 == 0) ? 0x07FF : COLOR_WHITE);
        }
        for (int mx = max(0, oldX - 16); mx < min(240, oldX + 16); mx++) {
            int mh = 10 + (mx * 7 % 15) + (mx * 3 % 8);
            int dy = max(140 - mh, oldY - 16);
            int h = min(140, oldY + 16) - dy;
            if (h > 0) lcd.drawFastVLine(mx, dy, h, 0x1004);
        }
        if (oldY + 16 > 140) {
            int gy = max(140, oldY - 16);
            lcd.fillRect(oldX - 16, gy, 32, oldY + 16 - gy, 0x2104);
        }
        if (oldX + 16 > ufoX + 10 && oldX - 16 < ufoX + 70 && oldY + 16 > ufoY - 20) {
            lcd.fillRect(ufoX + 15, ufoY - 20, 50, 30, 0x4208);
            for (int i = 0; i < 5; i++) lcd.drawFastHLine(ufoX + 15, ufoY - 20 + i * 6, 50, 0x2104);
            lcd.drawRect(ufoX + 20, ufoY - 15, 40, 20, 0x1082);
            lcd.fillRect(ufoX + 10, ufoY + 10, 60, 30, 0x3186);
            for (int i = 0; i < 6; i++) lcd.drawFastVLine(ufoX + 15 + i * 8, ufoY + 10, 30, 0x1082);
            lcd.drawFastHLine(ufoX + 10, ufoY + 25, 60, 0x2104);
            lcd.fillRect(ufoX + 25, ufoY + 30, 30, 10, 0x2104);
            lcd.fillRect(ufoX + 30, ufoY + 10, 20, 20, COLOR_BLACK);
            lcd.drawRect(ufoX + 29, ufoY + 9, 22, 22, 0x07FF);
            lcd.drawFastVLine(ufoX + 39, ufoY - 20, 30, 0x07FF);
            lcd.drawFastVLine(ufoX + 40, ufoY - 20, 30, 0x03FF);
        }
        if (ufoAnimState < 3) characterDraw(ufoAnimX, ufoAnimY, 0, 2);
        drawAlienTowerDynamic(ufoX, ufoY, now);
        drawAlienGroundAndEntities(now, false);
        return;
    }

    if (cg2Phase == 12) {
        if (cg2Dirty || now - lastCharMs < 40) return;
        lastCharMs = now;
        int old = warriorAnimX;
        warriorAnimX += 6;
        if (warriorAnimX > 250) { cg2Phase = 13; cg2Dirty = true; return; }
        drawArmoryBgRect(old - 25, 105 - 28, 50, 60);
        characterDraw(warriorAnimX, 105, 0, 3);
        return;
    }

    if (cg2Phase == 13) {
        if (cg2Dirty || now - lastCharMs < 40) return;
        lastCharMs = now;
        int oc = commanderAnimX, oy = cageAnimY;
        bool ani = false;

        if (commanderAnimX > 130) {
            commanderAnimX -= 4;
            if (commanderAnimX < 130) commanderAnimX = 130;
            ani = true;
        } else if (!isCageDropped) {
            cageAnimY += 12;
            if (cageAnimY >= 105) {
                cageAnimY = 105;
                isCageDropped = true;
                cg2PhaseTime = now;
            }
            ani = true;
        } else if (now - cg2PhaseTime > 500) {
            cg2Phase = 14;
            cg2Dirty = true;
        }

        if (ani) {
            if (oy != cageAnimY) drawArmoryBgRect(50 - 20, oy - 30, 42, 62);
            if (oc != commanderAnimX) drawArmoryBgRect(oc - 25, 105 - 26, 52, 52);
            characterDraw(50, 105, 0, 3);
            int cx = 50 - 18, cy = cageAnimY - 28;
            lcd.fillRect(cx, cy, 36, 56, COLOR_BLACK);
            for (int i = 0; i < 6; i++) lcd.drawFastVLine(cx + 3 + i * 6, cy, 56, COLOR_RED);
            lcd.drawRect(cx, cy, 36, 56, COLOR_YELLOW);
            lcd.drawRect(cx - 1, cy - 1, 38, 58, COLOR_YELLOW);
            drawAlien(commanderAnimX, 105, 3);
            lcd.fillRect(commanderAnimX - 6, 105 - 25, 12, 6, COLOR_YELLOW);
            lcd.fillRect(commanderAnimX - 20, 105 - 10, 8, 10, COLOR_YELLOW);
            lcd.fillRect(commanderAnimX + 12, 105 - 10, 8, 10, COLOR_YELLOW);
        }
        return;
    }

    if (cg2Phase == 25) {
        if (cg2Dirty || now - lastCharMs < 40) return;
        lastCharMs = now;
        int old = priestAnimX;
        bool ani = false;
        if (priestAnimX > 190) {
            priestAnimX -= 4;
            if (priestAnimX < 190) priestAnimX = 190;
            ani = true;
        } else if (now - cg2PhaseTime > 300) {
            cg2Phase = 16;
            cg2Dirty = true;
        }
        if (ani) {
            drawArmoryBgRect(old - 5, 95, 30, 45);
            if (priestAnimX < 240) characterDrawPriest(priestAnimX, 105);
        }
        return;
    }

    if (cg2Phase == 18) {
        if (cg2Dirty || now - lastCharMs < 40) return;
        lastCharMs = now;
        int old = cageAnimY;
        bool ani = false;

        if (cageAnimY > -60) {
            cageAnimY -= 12;
            if (cageAnimY < -60) cageAnimY = -60;
            ani = true;
        }

        if (ani) {
            drawArmoryBgRect(50 - 20, old - 30, 42, 62);
            characterDraw(50, 105, 0, 3);
            int cx = 50 - 18, cy = cageAnimY - 28;
            lcd.fillRect(cx, cy, 36, 56, COLOR_BLACK);
            for (int i = 0; i < 6; i++) lcd.drawFastVLine(cx + 3 + i * 6, cy, 56, COLOR_RED);
            lcd.drawRect(cx, cy, 36, 56, COLOR_YELLOW);
            lcd.drawRect(cx - 1, cy - 1, 38, 58, COLOR_YELLOW);
        } else {
            uint32_t el = now - cg2PhaseTime;
            if (el >= 500 && el < 1200) {
                if ((el / 50) % 2 == 0) {
                    drawLightningLine(130, 85, 190, 105, COLOR_WHITE);
                    drawLightningLine(130, 85, 195, 100, COLOR_CYAN);
                } else {
                    drawArmoryBgRect(130, 75, 85, 60);
                    drawAlien(130, 105, 3, false);
                    lcd.fillRect(124, 80, 12, 6, COLOR_YELLOW);
                    lcd.fillRect(110, 95, 8, 10, COLOR_YELLOW);
                    lcd.fillRect(142, 95, 8, 10, COLOR_YELLOW);
                    characterDraw(190 + (millis() % 6 - 3), 105 + (millis() % 4 - 2), 2, 3);
                }
            } else if (el >= 1200 && el < 1400) {
                drawArmoryBgRect(130, 75, 85, 60);
                drawAlien(130, 105, 3, false);
                lcd.fillRect(124, 80, 12, 6, COLOR_YELLOW);
                lcd.fillRect(110, 95, 8, 10, COLOR_YELLOW);
                lcd.fillRect(142, 95, 8, 10, COLOR_YELLOW);
                drawLightningLine(130, 85, 190, 105, COLOR_WHITE);
                drawLightningLine(130, 85, 192, 103, COLOR_WHITE);
                drawLightningLine(130, 85, 188, 107, COLOR_WHITE);
                drawLightningLine(130, 85, 195, 100, COLOR_CYAN);
                drawLightningLine(130, 85, 185, 110, COLOR_CYAN);
                lcd.fillRect(175, 90, 30, 30, COLOR_WHITE);
                lcd.fillRect(180, 95, 20, 20, COLOR_CYAN);
            } else if (el >= 1400 && el < 2500) {
                if (!isPriestDead) {
                    isPriestDead = true;
                    drawArmoryBgRect(130, 75, 85, 60);
                    drawAlien(130, 105, 3, true);
                    lcd.fillRect(124, 80, 12, 6, COLOR_YELLOW);
                    lcd.fillRect(110, 95, 8, 10, COLOR_YELLOW);
                    lcd.fillRect(142, 95, 8, 10, COLOR_YELLOW);
                    characterDrawSkull(185, 110);
                }
            } else if (el >= 2500) {
                cg2Phase = 19;
                cg2Dirty = true;
            }
        }
        return;
    }

    if (cg2Phase == 19 || cg2Phase == 20) {
        if (cg2Dirty || textDone || now - lastCharMs < 30) return;
        lastCharMs = now;
        const char* t = dialogs[3].text;
        int len = strlen(t);
        if (textCharIdx < len) {
            int l = 0, c = 0;
            for (int i = 0; i < textCharIdx; i++) {
                if (t[i] == '\n') { l++; c = 0; }
                else c++;
            }
            if (t[textCharIdx] != '\n')
                lcd.drawChar5x7(15 + c * 6, 160 + l * 10, t[textCharIdx], COLOR_WHITE, COLOR_BG_PANEL, 1);
            textCharIdx++;
        } else {
            textDone = true;
            lcd.drawText(160, 218, "[A] Next", COLOR_GRAY, COLOR_BG_PANEL, 1);
        }
        return;
    }

    if (cg2Phase == 23) {
        uint32_t el = now - cg2PhaseTime;
        if (el >= 16150) {
            cg2Phase = 30;
            ufoAnimState = 1;
            cg2Dirty = true;
        } else {
            int ch = (el * 214) / 14940;
            if (ch > 213) ch = 213;
            if (ch != ufoAnimState) {
                ufoAnimState = ch;
                cg2Dirty = true;
            }
        }
        return;
    }

    if (cg2Phase == 30) {
        uint32_t el = now - cg2PhaseTime;
        int sz = 1;
        if (el >= 29900) sz = 14;
        else if (el >= 28000) sz = 12;
        else if (el >= 26170) sz = 10;
        else if (el >= 24340) sz = 8;
        else if (el >= 22510) sz = 6;
        else if (el >= 20620) sz = 4;
        else if (el >= 18820) sz = 2;
        else sz = 1;

        if (sz != ufoAnimState) {
            ufoAnimState = sz;
            cg2Dirty = true;
        }
        if (el > 31900) {
            cg2Phase = 31;
            cg2PhaseTime = now;
            cg2Dirty = true;
            textDone = false;
        }
        return;
    }

    if (cg2Phase == 31) {
        if (!textDone && now - cg2PhaseTime > 2000) {
            textDone = true;
            cg2Dirty = true;
        }
        return;
    }

    if (cg2Dirty || textDone) return;

    int dlg = -1;
    if (cg2Phase == 10 || cg2Phase == 11) dlg = 0;
    else if (cg2Phase == 14 || cg2Phase == 15) dlg = 1;
    else if (cg2Phase == 16 || cg2Phase == 17) dlg = 2;
    else if (cg2Phase == 21 || cg2Phase == 22) dlg = 4;
    if (dlg == -1) { textDone = true; return; }

    const char* t = dialogs[dlg].text;
    int len = strlen(t);
    if (now - lastCharMs >= 30) {
        lastCharMs = now;
        if (textCharIdx < len) {
            int l = 0, c = 0;
            for (int i = 0; i < textCharIdx; i++) {
                if (t[i] == '\n') { l++; c = 0; }
                else c++;
            }
            if (t[textCharIdx] != '\n')
                lcd.drawChar5x7(15 + c * 6, 160 + l * 10, t[textCharIdx], COLOR_WHITE, COLOR_BG_PANEL, 1);
            textCharIdx++;
        } else {
            textDone = true;
            lcd.drawText(160, 218, "[A] Next", COLOR_GRAY, COLOR_BG_PANEL, 1);
        }
    }
}

bool cg2Update() {
    static bool prevA = false;
    static uint32_t last = 0;
    bool curA = (digitalRead(BTN_A) == LOW);
    bool press = curA && !prevA;
    prevA = curA;

    if (press && millis() - last < 300) press = false;
    else if (press) last = millis();
    if (!press) return false;

    if (cg2Phase < 10) {
        cg2Phase = 10;
        cg2Dirty = true;
        return false;
    }

    if (!textDone) {
        int dlg = -1;
        if (cg2Phase == 10 || cg2Phase == 11) dlg = 0;
        else if (cg2Phase == 14 || cg2Phase == 15) dlg = 1;
        else if (cg2Phase == 16 || cg2Phase == 17) dlg = 2;
        else if (cg2Phase == 19 || cg2Phase == 20) dlg = 3;
        else if (cg2Phase == 21 || cg2Phase == 22) dlg = 4;

        if (dlg != -1) {
            const char* txt = dialogs[dlg].text;
            int len = strlen(txt);
            while (textCharIdx < len) {
                int l = 0, c = 0;
                for (int i = 0; i < textCharIdx; i++) {
                    if (txt[i] == '\n') { l++; c = 0; }
                    else c++;
                }
                if (txt[textCharIdx] != '\n')
                    lcd.drawChar5x7(15 + c * 6, 160 + l * 10, txt[textCharIdx], COLOR_WHITE, COLOR_BG_PANEL, 1);
                textCharIdx++;
            }
            textDone = true;
            lcd.drawText(160, 218, "[A] Next", COLOR_GRAY, COLOR_BG_PANEL, 1);
        }
    } else {
        if (cg2Phase == 10 || cg2Phase == 11) cg2Phase = 12;
        else if (cg2Phase == 14 || cg2Phase == 15) {
            cg2Phase = 25;
            cg2PhaseTime = now;
        } else if (cg2Phase == 16 || cg2Phase == 17) {
            cg2Phase = 18;
            cg2PhaseTime = now;
            g_playerCorrupted = true;
        } else if (cg2Phase == 19 || cg2Phase == 20) {
            cg2Phase = 21;
            textDone = false;
            textCharIdx = 0;
        } else if (cg2Phase == 21 || cg2Phase == 22) {
            cg2Phase = 23;
            textDone = false;
            playTransmissionLogMusic();
            cg2PhaseTime = now;
            ufoAnimState = 0;
        } else if (cg2Phase == 31 && textDone) return true;
        cg2Dirty = true;
    }
    return false;
}