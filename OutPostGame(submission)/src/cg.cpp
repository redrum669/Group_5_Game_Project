#include "cg.h"
#include "st7789.h"
#include "config.h"
#include <stdlib.h>
#include <Arduino.h>

// Cutscene global runtime state
static int      cgPhase     = 0;    // 0: story text, 1: invasion scene, 2: earth destroy, 3: lore text
static uint32_t cgStartTime = 0;
static bool     cgDirty     = true; // Flag for full redraw on phase change

static const uint32_t invasionDuration = 4000;  // 4 seconds total
static const uint32_t textDuration     = 6000;  // 6 seconds total

// ==================== Phase 0: Oath of the Altar ==========================
// Story text line structure with custom color
struct StoryEntry { const char* text; uint16_t color; };

#define STORY_N_LINES   20
#define STORY_BLOOD_START 13   // Lines from this index use blood effect
#define STORY_LINE_Y0    5
#define STORY_LINE_H     11

// Script lines for opening narrative
static const StoryEntry storyLines[STORY_N_LINES] = {
    { "They gave us embers",           0xC618 },
    { "and called it light --",        0xC618 },
    { "war, nothingness, and",         0xC618 },
    { "the slow death",                0xC618 },
    { "of green things.",              0xC618 },
    { " ",                             0x0000 },
    { "Yet from their hollow temples,",0xC618 },
    { "we learned to kneel",           0xC618 },
    { "to something greater,",         0xC618 },
    { "and in the silence,",           0xC618 },
    { "a desperate hope was born.",    0xFFFF },
    { " ",                             0x0000 },
    { "But know this:",                0xFDA0 },
    { "every god carved in fear",      0xF800 },
    { "shall be unmade.",              0xF800 },
    { "We will burn their heavens,",   0xF800 },
    { "and salt the earth",            0xF800 },
    { "where their altars stood.",     0xF800 },
    { " ",                             0x0000 },
    { "  --Oath of the Altar",         0xFDA0 },
};

// Typewriter text animation state
static int      storyLine    = 0;
static int      storyChar    = 0;
static uint32_t storyMs      = 0;
static bool     storyAllDone = false;

// Blood drip effect configuration
#define MAX_DRIPS 24
struct Drip { int16_t x, topY, len, maxLen; bool active; };
static Drip     drips[MAX_DRIPS];
static uint32_t dripTickMs  = 0;
static bool     dripsInited = false;

// ==================== Phase 2: Earth Destruction ==========================
#define MISSILE_STEPS 30
#define MTRAIL        4

// Projectile for planetary bombardment
struct Missile {
    int16_t  sx, sy, ex, ey;
    int16_t  trailX[MTRAIL], trailY[MTRAIL];
    int      step;
    uint32_t launchDelay;
    bool     active, done;
};

// Explosion animation
struct Boom   { int16_t cx, cy; int r, maxR; bool active; };

// Smoke particle after impact
struct SmokeP { int16_t x, y;  uint16_t color; bool active; };

static Missile  missiles[3];
static Boom     booms[3];
#define MAX_SMOKE 10
static SmokeP   smokeP[MAX_SMOKE];

static uint32_t destroyStartMs = 0;
static uint32_t destroyTickMs  = 0;
static uint32_t smokeTickMs    = 0;
static int      burnStep       = 0;
static bool     destroyInited  = false;

// ------------------------------------------------------------------
// Initialize cutscene system and reset all state
void cgInit() {
    Serial.println("PLAY_BGM");
    pinMode(PC13, INPUT);

    cgPhase      = 0;
    cgStartTime  = 0;
    cgDirty      = true;

    storyLine    = 0;
    storyChar    = 0;
    storyMs      = 0;
    storyAllDone = false;
    dripsInited  = false;
    dripTickMs   = 0;

    for (int i = 0; i < MAX_DRIPS; i++) {
        drips[i].active = false;
    }

    destroyInited  = false;
    destroyStartMs = 0;
    destroyTickMs  = 0;
    smokeTickMs    = 0;
    burnStep       = 0;

    for (int i = 0; i < 3; i++) {
        missiles[i].active = false;
        missiles[i].done = false;
        booms[i].active = false;
    }
    for (int i = 0; i < MAX_SMOKE; i++) {
        smokeP[i].active = false;
    }
}

// Return true if full redraw is needed
bool cgNeedsRedraw() {
    if (cgDirty) {
        cgDirty = false;
        return true;
    }
    return false;
}

// ==================== Drawing Helpers =============================
// Basic graphics primitives missing in ST7789 library

// Bresenham line with fast horizontal/vertical paths
static void cgDrawLine(int x0, int y0, int x1, int y1, uint16_t color) {
    if (y0 == y1) {
        int lx = x0 < x1 ? x0 : x1;
        lcd.drawFastHLine(lx, y0, abs(x1 - x0) + 1, color);
        return;
    }
    if (x0 == x1) {
        int ly = y0 < y1 ? y0 : y1;
        lcd.drawFastVLine(x0, ly, abs(y1 - y0) + 1, color);
        return;
    }
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while (true) {
        lcd.drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

// Filled circle using horizontal lines
static void cgFillCircle(int cx, int cy, int r, uint16_t color) {
    int x = 0, y = r, d = 3 - 2 * r;
    while (x <= y) {
        lcd.drawFastHLine(cx - y, cy - x, 2 * y + 1, color);
        lcd.drawFastHLine(cx - y, cy + x, 2 * y + 1, color);
        lcd.drawFastHLine(cx - x, cy - y, 2 * x + 1, color);
        lcd.drawFastHLine(cx - x, cy + y, 2 * x + 1, color);
        if (d < 0) d += 4 * x + 6;
        else       { d += 4 * (x - y) + 10; y--; }
        x++;
    }
}

// Circle outline using midpoint algorithm
static void cgDrawCircle(int cx, int cy, int r, uint16_t color) {
    int x = 0, y = r, d = 3 - 2 * r;
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
        else       { d += 4 * (x - y) + 10; y--; }
        x++;
    }
}

// ------------------------------------------------------------------
// Draw static starfield background once
static void drawStars() {
    for (int i = 0; i < 180; i++) {
        int x = random(0, LCD_WIDTH);
        int y = random(0, LCD_HEIGHT);
        uint16_t color = (random(0, 3) == 0) ? COLOR_WHITE : COLOR_LIGHTGRAY;
        lcd.drawPixel(x, y, color);
    }
    for (int i = 0; i < 12; i++) {
        int x = random(0, LCD_WIDTH);
        int y = random(0, LCD_HEIGHT);
        lcd.fillRect(x, y, 2, 2, COLOR_WHITE);
    }
}

// ------------------------------------------------------------------
// Draw Earth planet with continents and glow
static void drawEarth() {
    int cx = 58, cy = 130, r = 32;
    cgFillCircle(cx, cy, r, COLOR_BLUE);
    cgDrawCircle(cx, cy, r, COLOR_CYAN);

    lcd.fillRect(cx-12, cy-8,  14, 9,  COLOR_DARKGREEN);
    lcd.fillRect(cx+4,  cy+8,  11, 7,  COLOR_DARKGREEN);
    lcd.fillRect(cx-6,  cy+14,  8, 5,  COLOR_DARKGREEN);

    cgDrawCircle(cx, cy, r+2, 0x03FF);
}

// ------------------------------------------------------------------
// Draw alien mothership with halo and details
static void drawShip() {
    int sx = 185, sy = 65;
    cgDrawCircle(sx, sy, 24, COLOR_GOLD);
    cgDrawCircle(sx, sy, 26, 0x8400);

    lcd.fillRect(sx-18, sy-5, 36, 10, COLOR_DARKGRAY);
    lcd.fillRect(sx-12, sy-9, 24,  8, COLOR_GRAY);
    lcd.fillRect(sx-6,  sy-13, 12, 6, COLOR_LIGHTGRAY);

    cgFillCircle(sx, sy-5, 7, COLOR_CYAN);
    cgDrawCircle(sx, sy-5, 7, COLOR_WHITE);

    lcd.fillRect(sx-14, sy+4, 4, 3, COLOR_ORANGE);
    lcd.fillRect(sx-5,  sy+4, 4, 3, COLOR_ORANGE);
    lcd.fillRect(sx+4,  sy+4, 4, 3, COLOR_ORANGE);
    lcd.fillRect(sx+10, sy+4, 4, 3, COLOR_ORANGE);

    cgDrawCircle(sx, sy-32, 9, COLOR_MAGENTA);
    cgFillCircle(sx, sy-32, 4, COLOR_MAGENTA);
    cgFillCircle(sx, sy-32, 2, COLOR_YELLOW);
    cgDrawLine(sx-7, sy-32, sx+7, sy-32, COLOR_MAGENTA);
    cgDrawLine(sx,   sy-41, sx,   sy-23, COLOR_MAGENTA);
}

// ------------------------------------------------------------------
// Draw energy beams from ship to planet
static void drawBeams() {
    int sx = 185, sy = 65;
    int ex = 58,  ey = 130;

    cgDrawLine(sx-8, sy+6, ex+22, ey-14, COLOR_YELLOW);
    cgDrawLine(sx,   sy+6, ex+10, ey+8,  COLOR_YELLOW);
    cgDrawLine(sx+8, sy+6, ex+14, ey-22, COLOR_YELLOW);

    cgDrawLine(sx-7, sy+6, ex+21, ey-14, COLOR_ORANGE);
    cgDrawLine(sx+7, sy+6, ex+13, ey-22, COLOR_ORANGE);

    lcd.fillRect(ex+20, ey-12, 4, 4, COLOR_WHITE);
    lcd.fillRect(ex+8,  ey+6,  4, 4, COLOR_WHITE);
    lcd.fillRect(ex+12, ey-20, 3, 3, COLOR_YELLOW);
    lcd.fillRect(ex+24, ey-16, 2, 2, COLOR_CYAN);
}

// ==================== Phase 0: Story Text Helpers =========================

// Clear screen for story mode
static void drawStoryInit() {
    lcd.fillScreen(COLOR_BLACK);
    lcd.drawText(78, 228, "[A] Skip", 0x39E7, COLOR_BLACK, 1);
}

static void updateDestroyAnim();

// ------------------------------------------------------------------
// Incremental typewriter and blood effect rendering
void cgDrawUpdate() {
    if (cgPhase == 2) {
        updateDestroyAnim();
        return;
    }
    if (cgPhase != 0) return;

    uint32_t now = millis();

    // Animate blood drips
    if (now - dripTickMs >= 90) {
        dripTickMs = now;
        for (int i = 0; i < MAX_DRIPS; i++) {
            if (!drips[i].active) continue;

            int16_t py = drips[i].topY + drips[i].len;
            if (drips[i].len < drips[i].maxLen && py < 230) {
                lcd.fillRect(drips[i].x, py, 2, 2, 0xA000);
                lcd.drawFastHLine(drips[i].x, py + 2, 2, 0x6000);
                drips[i].len += 2;
            } else {
                drips[i].active = false;
            }
        }
    }

    if (storyAllDone) return;

    // Typewriter timing control
    if (storyMs == 0) storyMs = now;
    uint16_t delayTime = (storyLine >= 12) ? 40 : 64;

    if (now - storyMs < delayTime) return;
    storyMs = now;

    // Draw next character
    if (storyLine < STORY_N_LINES) {
        const char* text = storyLines[storyLine].text;
        int lineLen = strlen(text);

        if (lineLen == 0 || text[0] == ' ') {
            storyLine++;
            storyChar = 0;
            return;
        }

        int lineY = STORY_LINE_Y0 + storyLine * STORY_LINE_H;
        int lineX = (240 - lineLen * 6) / 2;
        uint16_t col = storyLines[storyLine].color;

        if (storyChar < lineLen) {
            char c = text[storyChar];
            lcd.drawChar5x7(lineX + storyChar * 6, lineY, c, col, COLOR_BLACK, 1);

            // Spawn blood drips randomly on red lines
            if (col == 0xF800 && c != ' ' && random(100) < 15) {
                for (int i = 0; i < MAX_DRIPS; i++) {
                    if (!drips[i].active) {
                        drips[i].active = true;
                        drips[i].x = lineX + storyChar * 6 + random(4);
                        drips[i].topY = lineY + 8;
                        drips[i].len = 0;
                        drips[i].maxLen = 6 + random(30);
                        break;
                    }
                }
            }
            storyChar++;
        } else {
            storyLine++;
            storyChar = 0;
        }
    }

    // Show continue prompt when finished
    if (storyLine >= STORY_N_LINES) {
        storyAllDone = true;
        lcd.fillRect(0, 224, 240, 16, COLOR_BLACK);
        lcd.drawText(43, 228, "[A] Continue", COLOR_GRAY, COLOR_BLACK, 1);
    }
}

// ==================== Phase 2: Earth Destruction ==========================
#define EARTH_CX 58
#define EARTH_CY 130

// Initialize missile/explosion state
static void initDestroyAnim() {
    if (destroyInited) return;
    destroyInited  = true;
    destroyStartMs = millis();
    destroyTickMs  = millis();
    smokeTickMs    = millis();
    burnStep = 0;

    int16_t lx[3] = {171, 182, 193};
    int16_t ly[3] = { 71,  71,  71};
    int16_t tx[3] = {EARTH_CX+22, EARTH_CX+10, EARTH_CX+15};
    int16_t ty[3] = {EARTH_CY-15, EARTH_CY+10, EARTH_CY-22};
    uint32_t delays[3] = {0, 500, 950};

    for (int i = 0; i < 3; i++) {
        missiles[i].sx = lx[i];
        missiles[i].sy = ly[i];
        missiles[i].ex = tx[i];
        missiles[i].ey = ty[i];
        missiles[i].step = 0;
        missiles[i].launchDelay = delays[i];
        missiles[i].active = false;
        missiles[i].done   = false;
        for (int j = 0; j < MTRAIL; j++) {
            missiles[i].trailX[j] = lx[i];
            missiles[i].trailY[j] = ly[i];
        }
        booms[i].cx = tx[i];
        booms[i].cy = ty[i];
        booms[i].r  = 0;
        booms[i].maxR = 18;
        booms[i].active = false;
    }
    for (int i = 0; i < MAX_SMOKE; i++) {
        smokeP[i].active = false;
    }
}

// Animate missile movement and trails
static void animMissiles(uint32_t elapsed) {
    static const uint16_t trailCol[MTRAIL] = {COLOR_WHITE, COLOR_YELLOW, COLOR_ORANGE, COLOR_BLACK};
    for (int i = 0; i < 3; i++) {
        if (missiles[i].done) continue;
        if (elapsed < missiles[i].launchDelay) continue;
        missiles[i].active = true;

        lcd.drawPixel(missiles[i].trailX[MTRAIL-1], missiles[i].trailY[MTRAIL-1], COLOR_BLACK);
        for (int j = MTRAIL-1; j > 0; j--) {
            missiles[i].trailX[j] = missiles[i].trailX[j-1];
            missiles[i].trailY[j] = missiles[i].trailY[j-1];
        }

        int s = missiles[i].step;
        int16_t nx = missiles[i].sx + (int16_t)((long)(missiles[i].ex - missiles[i].sx) * s / MISSILE_STEPS);
        int16_t ny = missiles[i].sy + (int16_t)((long)(missiles[i].ey - missiles[i].sy) * s / MISSILE_STEPS);
        missiles[i].trailX[0] = nx;
        missiles[i].trailY[0] = ny;

        lcd.fillRect(nx-1, ny-1, 3, 3, trailCol[0]);
        for (int j = 1; j < MTRAIL-1; j++) {
            lcd.drawPixel(missiles[i].trailX[j], missiles[i].trailY[j], trailCol[j]);
        }

        missiles[i].step++;
        if (missiles[i].step >= MISSILE_STEPS) {
            for (int j = 0; j < MTRAIL; j++) {
                lcd.drawPixel(missiles[i].trailX[j], missiles[i].trailY[j], COLOR_BLACK);
            }
            missiles[i].done   = true;
            booms[i].active    = true;
            booms[i].r         = 1;
        }
    }
}

// Animate explosion rings
static void animExplosions() {
    for (int i = 0; i < 3; i++) {
        if (!booms[i].active) continue;
        uint16_t c = COLOR_WHITE;
        if (booms[i].r >  4) c = COLOR_YELLOW;
        if (booms[i].r >  8) c = COLOR_ORANGE;
        if (booms[i].r > 12) c = 0xA000;
        cgDrawCircle(booms[i].cx, booms[i].cy, booms[i].r, c);
        booms[i].r++;
        if (booms[i].r > booms[i].maxR) booms[i].active = false;
    }
}

// Animate planet burning layers
static void animBurnEarth(uint32_t elapsed) {
    if (elapsed < 2400) return;
    static const uint16_t burnCol[6] = {0xC340, 0x8200, 0x6180, 0x4108, 0x2104, 0x1082};
    static const int      burnRad[6] = {30, 26, 22, 17, 11, 6};
    int target = (int)((elapsed - 2400) / 320);
    if (target > 6) target = 6;
    while (burnStep < target) {
        int idx = burnStep < 6 ? burnStep : 5;
        cgFillCircle(EARTH_CX, EARTH_CY, burnRad[idx], burnCol[idx]);
        burnStep++;
    }
    if (burnStep >= 6) {
        cgFillCircle(EARTH_CX, EARTH_CY, 5, COLOR_BLACK);
        cgDrawCircle(EARTH_CX, EARTH_CY, 32, 0x4208);
    }
}

// Animate rising smoke particles
static void animSmoke(uint32_t elapsed) {
    if (elapsed < 2800) return;
    uint32_t now = millis();
    if (now - smokeTickMs < 160) return;
    smokeTickMs = now;

    static const int16_t ox[8] = {-28,  0, 28, 20,-20,-15, 15,  0};
    static const int16_t oy[8] = {  0,-28,  0,-20, 20, -5,  5, 20};
    for (int i = 0; i < MAX_SMOKE; i++) {
        if (!smokeP[i].active) {
            int ang = (int)random(0, 8);
            smokeP[i].x     = EARTH_CX + ox[ang];
            smokeP[i].y     = EARTH_CY + oy[ang];
            smokeP[i].color = (random(0, 2) == 0) ? (uint16_t)0x4208 : (uint16_t)0x8410;
            smokeP[i].active = true;
            lcd.fillRect(smokeP[i].x, smokeP[i].y, 3, 3, smokeP[i].color);
            break;
        }
    }

    for (int i = 0; i < MAX_SMOKE; i++) {
        if (!smokeP[i].active) continue;
        lcd.fillRect(smokeP[i].x, smokeP[i].y, 3, 3, COLOR_BLACK);
        smokeP[i].y -= 2;
        if (smokeP[i].y < 55 || smokeP[i].y > 220) {
            smokeP[i].active = false;
        } else {
            uint16_t fade = (smokeP[i].y < 90) ? (uint16_t)0x2104 : smokeP[i].color;
            lcd.fillRect(smokeP[i].x, smokeP[i].y, 3, 3, fade);
        }
    }
}

// Main animation loop for destruction scene
static void updateDestroyAnim() {
    initDestroyAnim();
    uint32_t now     = millis();
    uint32_t elapsed = now - destroyStartMs;
    if (now - destroyTickMs < 55) return;
    destroyTickMs = now;

    animMissiles(elapsed);
    animExplosions();
    animBurnEarth(elapsed);
    animSmoke(elapsed);

    if (elapsed >= 6200) {
        cgPhase     = 3;
        cgStartTime = millis();
        cgDirty     = true;
    }
}

// ------------------------------------------------------------------
// Draw full invasion scene
static void drawInvasionScene() {
    lcd.fillScreen(COLOR_BLACK);
    drawStars();
    drawEarth();
    drawShip();
    drawBeams();

    lcd.drawText(12, 10,  "O U T P O S T", COLOR_GOLD,   COLOR_BLACK, 1);
    lcd.drawText(20, 22,  "THE  ARRIVAL",  COLOR_RED,    COLOR_BLACK, 1);
    lcd.drawText(22, 222, "Press [A] to continue", COLOR_GRAY, COLOR_BLACK, 1);
}

// Draw lore text scene
static void drawTextScene() {
    lcd.fillScreen(COLOR_BLACK);
    drawStars();

    lcd.fillRect(10, 30, 220, 175, COLOR_BG_PANEL);
    lcd.drawRect(10, 30, 220, 175, COLOR_GOLD);
    lcd.drawRect(11, 31, 218, 173, 0x4208);

    lcd.drawText(30, 42, "-- TRANSMISSION LOG --", COLOR_GOLD, COLOR_BG_PANEL, 1);
    cgDrawLine(12, 53, 228, 53, COLOR_GOLD);

    lcd.drawText(18, 62,  "They came not for our",   COLOR_WHITE,      COLOR_BG_PANEL, 1);
    lcd.drawText(18, 76,  "resources or our land.",   COLOR_WHITE,      COLOR_BG_PANEL, 1);
    lcd.drawText(18, 90,  "They came with a creed.",  COLOR_LIGHTGRAY,  COLOR_BG_PANEL, 1);
    lcd.drawText(18, 110, "Cities fell. Skies burned.", COLOR_RED,      COLOR_BG_PANEL, 1);
    lcd.drawText(18, 124, "But in the ruins,",         COLOR_LIGHTGRAY, COLOR_BG_PANEL, 1);
    lcd.drawText(18, 138, "survivors hold on.",        COLOR_LIGHTGRAY, COLOR_BG_PANEL, 1);
    lcd.drawText(18, 158, "You are one of them.",      COLOR_CYAN,      COLOR_BG_PANEL, 1);
    lcd.drawText(18, 172, "Find the outposts.",        COLOR_GOLD,      COLOR_BG_PANEL, 1);
    lcd.drawText(18, 186, "Keep the hope alive.",      COLOR_GOLD,      COLOR_BG_PANEL, 1);

    cgDrawLine(12, 198, 228, 198, COLOR_GOLD);

    cgFillCircle(120, 215, 10, 0x4010);
    cgDrawCircle(120, 215, 10, COLOR_MAGENTA);
    cgFillCircle(120, 215,  4, COLOR_MAGENTA);
    cgFillCircle(120, 215,  2, COLOR_YELLOW);
    cgDrawLine(120, 203, 120, 227, COLOR_MAGENTA);
    cgDrawLine(108, 215, 132, 215, COLOR_MAGENTA);

    lcd.drawText(30, 232, "Press [A] to continue", COLOR_GRAY, COLOR_BLACK, 1);
}

// ------------------------------------------------------------------
// Redraw entire current phase
void cgDraw() {
    if (cgPhase == 0) {
        drawStoryInit();
    } else if (cgPhase == 1) {
        drawInvasionScene();
    } else if (cgPhase == 2) {
        drawInvasionScene();
        destroyInited = false;
    } else {
        drawTextScene();
    }
}

// ------------------------------------------------------------------
// Update input and phase progression
bool cgUpdate() {
    static bool prevBtnA = false;
    static bool prevBtnBlue = false;

    if (cgStartTime == 0) {
        cgStartTime = millis();
        randomSeed(micros());
        prevBtnBlue = digitalRead(PC13);
    }

    bool curBtnBlue = digitalRead(PC13);
    if (curBtnBlue == HIGH && prevBtnBlue == LOW) {
        Serial.println("STOP_MUSIC");
    }
    prevBtnBlue = curBtnBlue;

    bool curBtnA    = (digitalRead(BTN_A) == LOW);
    bool btnPressed = curBtnA && !prevBtnA;
    prevBtnA = curBtnA;

    if (!btnPressed) return false;

    // Advance through phases
    if (cgPhase == 0) {
        cgPhase     = 1;
        cgStartTime = millis();
        cgDirty     = true;
        return false;
    }
    if (cgPhase == 1) {
        cgPhase     = 2;
        cgStartTime = millis();
        cgDirty     = true;
        return false;
    }
    if (cgPhase == 2) {
        cgPhase     = 3;
        cgStartTime = millis();
        cgDirty     = true;
        return false;
    }

    // End of cutscene
    Serial.println("STOP_MUSIC");
    return true;
}