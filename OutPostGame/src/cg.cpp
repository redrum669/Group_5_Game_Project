#include "cg.h"
#include "st7789.h"
#include "config.h"
#include <stdlib.h>
#include <Arduino.h>

static int      cgPhase     = 0;    // 0: story text, 1: invasion scene, 2: earth destroy, 3: lore text
static uint32_t cgStartTime = 0;
static bool     cgDirty     = true; // true when phase changed and redraw needed

static const uint32_t invasionDuration = 4000;  // 4 seconds
static const uint32_t textDuration     = 6000;  // 6 seconds

// ==================== Phase 0: Oath of the Altar ==========================
struct StoryEntry { const char* text; uint16_t color; };

#define STORY_N_LINES   20
#define STORY_BLOOD_START 13   // first line index with blood effect
#define STORY_LINE_Y0    5
#define STORY_LINE_H     11

static const StoryEntry storyLines[STORY_N_LINES] = {
    { "They gave us embers",           0xC618 },  // light gray
    { "and called it light --",        0xC618 },
    { "war, nothingness, and",         0xC618 },
    { "the slow death",                0xC618 },
    { "of green things.",              0xC618 },
    { " ",                             0x0000 },
    { "Yet from their hollow temples,",0xC618 },
    { "we learned to kneel",           0xC618 },
    { "to something greater,",         0xC618 },
    { "and in the silence,",           0xC618 },
    { "a desperate hope was born.",    0xFFFF },  // white
    { " ",                             0x0000 },  // blank
    { "But know this:",                0xFDA0 },  // gold
    { "every god carved in fear",      0xF800 },  // red / blood
    { "shall be unmade.",              0xF800 },
    { "We will burn their heavens,",   0xF800 },
    { "and salt the earth",            0xF800 },
    { "where their altars stood.",     0xF800 },
    { " ",                             0x0000 },  // blank
    { "  --Oath of the Altar",         0xFDA0 },  // gold
};

// typewriter state
static int      storyLine    = 0;
static int      storyChar    = 0;
static uint32_t storyMs      = 0;
static bool     storyAllDone = false;

// blood drip state
#define MAX_DRIPS 24
struct Drip { int16_t x, topY, len, maxLen; bool active; };
static Drip     drips[MAX_DRIPS];
static uint32_t dripTickMs  = 0;
static bool     dripsInited = false;

// ==================== Phase 2: Earth Destruction ==========================
#define MISSILE_STEPS 30
#define MTRAIL        4
struct Missile {
    int16_t  sx, sy, ex, ey;
    int16_t  trailX[MTRAIL], trailY[MTRAIL];
    int      step;
    uint32_t launchDelay;
    bool     active, done;
};
struct Boom   { int16_t cx, cy; int r, maxR; bool active; };
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
void cgInit() {
    // Send a signal over Serial to the PC to start playing the music
    Serial.println("PLAY_BGM");
    pinMode(PC13, INPUT); // Initialize the blue button

    cgPhase      = 0;
    cgStartTime  = 0;
    cgDirty      = true;
    // Reset story typewriter state
    storyLine    = 0;
    storyChar    = 0;
    storyMs      = 0;
    storyAllDone = false;
    dripsInited  = false;
    dripTickMs   = 0;
    for (int i = 0; i < MAX_DRIPS; i++) drips[i].active = false;
    // Reset phase 2 destruction state
    destroyInited  = false;
    destroyStartMs = 0;
    destroyTickMs  = 0;
    smokeTickMs    = 0;
    burnStep       = 0;
    for (int i = 0; i < 3;         i++) { missiles[i].active = false; missiles[i].done = false; booms[i].active = false; }
    for (int i = 0; i < MAX_SMOKE; i++) smokeP[i].active = false;
}

bool cgNeedsRedraw() {
    if (cgDirty) { cgDirty = false; return true; }
    return false;
}

// ==================== Drawing helpers =============================
// ST7789 has no circle/line primitives — implement locally.

// Bresenham line (axis-aligned fast paths included)
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

// Filled circle — midpoint algorithm using horizontal scanlines
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

// Circle outline — midpoint algorithm using individual pixels
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
// Draw starfield — called once at scene start, not every frame
static void drawStars() {
    for (int i = 0; i < 180; i++) {
        int x = random(0, LCD_WIDTH);
        int y = random(0, LCD_HEIGHT);
        uint16_t color = (random(0, 3) == 0) ? COLOR_WHITE : COLOR_LIGHTGRAY;
        lcd.drawPixel(x, y, color);
    }
    // A few brighter star clusters
    for (int i = 0; i < 12; i++) {
        int x = random(0, LCD_WIDTH);
        int y = random(0, LCD_HEIGHT);
        lcd.fillRect(x, y, 2, 2, COLOR_WHITE);
    }
}

// ------------------------------------------------------------------
// Draw Earth (blue globe with green continent patches)
static void drawEarth() {
    int cx = 58, cy = 130, r = 32;
    cgFillCircle(cx, cy, r, COLOR_BLUE);
    cgDrawCircle(cx, cy, r, COLOR_CYAN);
    // Continents
    lcd.fillRect(cx-12, cy-8,  14, 9,  COLOR_DARKGREEN);
    lcd.fillRect(cx+4,  cy+8,  11, 7,  COLOR_DARKGREEN);
    lcd.fillRect(cx-6,  cy+14,  8, 5,  COLOR_DARKGREEN);
    // Atmosphere glow
    cgDrawCircle(cx, cy, r+2, 0x03FF);  // faint blue-cyan
}

// ------------------------------------------------------------------
// Draw alien mothership with ominous halo
static void drawShip() {
    int sx = 185, sy = 65;
    // Outer glow rings
    cgDrawCircle(sx, sy, 24, COLOR_GOLD);
    cgDrawCircle(sx, sy, 26, 0x8400);   // dark red outer ring
    // Ship body — saucer shape
    lcd.fillRect(sx-18, sy-5, 36, 10, COLOR_DARKGRAY);
    lcd.fillRect(sx-12, sy-9, 24,  8, COLOR_GRAY);
    lcd.fillRect(sx-6,  sy-13, 12, 6, COLOR_LIGHTGRAY);
    // Cockpit dome
    cgFillCircle(sx, sy-5, 7, COLOR_CYAN);
    cgDrawCircle(sx, sy-5, 7, COLOR_WHITE);
    // Engine glow dots underneath
    lcd.fillRect(sx-14, sy+4, 4, 3, COLOR_ORANGE);
    lcd.fillRect(sx-5,  sy+4, 4, 3, COLOR_ORANGE);
    lcd.fillRect(sx+4,  sy+4, 4, 3, COLOR_ORANGE);
    lcd.fillRect(sx+10, sy+4, 4, 3, COLOR_ORANGE);
    // Mystic symbol above (eye of the invaders)
    cgDrawCircle(sx, sy-32, 9, COLOR_MAGENTA);
    cgFillCircle(sx, sy-32, 4, COLOR_MAGENTA);
    cgFillCircle(sx, sy-32, 2, COLOR_YELLOW);
    cgDrawLine(sx-7, sy-32, sx+7, sy-32, COLOR_MAGENTA);
    cgDrawLine(sx,   sy-41, sx,   sy-23, COLOR_MAGENTA);
}

// ------------------------------------------------------------------
// Draw energy beams from ship toward Earth
static void drawBeams() {
    int sx = 185, sy = 65;
    int ex = 58,  ey = 130;
    // Three main beams (yellow energy)
    cgDrawLine(sx-8, sy+6, ex+22, ey-14, COLOR_YELLOW);
    cgDrawLine(sx,   sy+6, ex+10, ey+8,  COLOR_YELLOW);
    cgDrawLine(sx+8, sy+6, ex+14, ey-22, COLOR_YELLOW);
    // Beam glow (thin orange alongside)
    cgDrawLine(sx-7, sy+6, ex+21, ey-14, COLOR_ORANGE);
    cgDrawLine(sx+7, sy+6, ex+13, ey-22, COLOR_ORANGE);
    // Impact sparks on Earth surface
    lcd.fillRect(ex+20, ey-12, 4, 4, COLOR_WHITE);
    lcd.fillRect(ex+8,  ey+6,  4, 4, COLOR_WHITE);
    lcd.fillRect(ex+12, ey-20, 3, 3, COLOR_YELLOW);
    lcd.fillRect(ex+24, ey-16, 2, 2, COLOR_CYAN);
}

// ==================== Phase 0: Story text helpers =========================

// One-time black canvas for the story text
static void drawStoryInit() {
    lcd.fillScreen(COLOR_BLACK);
    // Faint skip hint — will be replaced with "[A] Continue" when done
    lcd.drawText(78, 228, "[A] Skip", 0x39E7, COLOR_BLACK, 1);
}

static void updateDestroyAnim();   // forward declaration

// ------------------------------------------------------------------
// Robust Phase 0 Typewriter State Machine
// ------------------------------------------------------------------
void cgDrawUpdate() {
    if (cgPhase == 2) { updateDestroyAnim(); return; }
    if (cgPhase != 0) return;

    uint32_t now = millis();

    // ==========================================
    // 1. Blood Drip Animation (Independent)
    // ==========================================
    if (now - dripTickMs >= 90) {
        dripTickMs = now;
        for (int i = 0; i < MAX_DRIPS; i++) {
            if (!drips[i].active) continue;

            int16_t py = drips[i].topY + drips[i].len;
            if (drips[i].len < drips[i].maxLen && py < 230) {
                // Draw thick drip (using fillRect for perfect bounds safety)
                lcd.fillRect(drips[i].x, py, 2, 2, 0xA000); // Dark red body
                lcd.drawFastHLine(drips[i].x, py + 2, 2, 0x6000); // Tip
                drips[i].len += 2;
            } else {
                drips[i].active = false;
            }
        }
    }

    if (storyAllDone) return;

    // ==========================================
    // 2. Typewriter Text Rendering
    // ==========================================
    if (storyMs == 0) storyMs = now;
    // 15.71s total time -> minus ~1000ms boot delay = 14710ms for ~219 chars -> ~67ms per char
    // NOTE: PC music start is now instant on boot (in main.cpp) to eliminate reset delay.
    // However, the user wants this to happen 2.19 seconds earlier: (14710 - 2190) = 12520ms / 219 chars -> ~57ms
    uint16_t delayTime = (storyLine >= 12) ? 40 : 64;

    if (now - storyMs < delayTime) return;
    storyMs = now;

    // Advance to next line if needed
    if (storyLine < STORY_N_LINES) {
        const char* text = storyLines[storyLine].text;
        int lineLen = strlen(text);

        // Skip immediately if blank
        if (lineLen == 0 || text[0] == ' ') {
            storyLine++;
            storyChar = 0;
            return;
        }

        // Draw character
        int lineY = STORY_LINE_Y0 + storyLine * STORY_LINE_H;
        int lineX = (240 - lineLen * 6) / 2;
        uint16_t col = storyLines[storyLine].color;

        if (storyChar < lineLen) {
            char c = text[storyChar];
            lcd.drawChar5x7(lineX + storyChar * 6, lineY, c, col, COLOR_BLACK, 1);

            // Randomly spawn a blood drip if this is a red blood character (and not space)
            if (col == 0xF800 && c != ' ' && random(100) < 15) { // 15% chance per char
                for (int i = 0; i < MAX_DRIPS; i++) {
                    if (!drips[i].active) {
                        drips[i].active = true;
                        drips[i].x = lineX + storyChar * 6 + random(4); // randomize x slightly within char
                        drips[i].topY = lineY + 8; // start right below the char
                        drips[i].len = 0;
                        drips[i].maxLen = 6 + random(30); // length of drip
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

    // Check completion
    if (storyLine >= STORY_N_LINES) {
        storyAllDone = true;
        lcd.fillRect(0, 224, 240, 16, COLOR_BLACK);
        lcd.drawText(43, 228, "[A] Continue", COLOR_GRAY, COLOR_BLACK, 1);
    }
}

// ==================== Phase 2: Earth Destruction Animation ================
#define EARTH_CX 58
#define EARTH_CY 130

static void initDestroyAnim() {
    if (destroyInited) return;
    destroyInited  = true;
    destroyStartMs = millis();
    destroyTickMs  = millis();
    smokeTickMs    = millis();
    burnStep = 0;

    // Launch from ship engine ports (SHIP_X=185, SHIP_Y=65)
    int16_t lx[3] = {171, 182, 193};
    int16_t ly[3] = { 71,  71,  71};
    // Impact on Earth surface (EARTH_CX=58, EARTH_CY=130)
    int16_t tx[3] = {EARTH_CX+22, EARTH_CX+10, EARTH_CX+15};
    int16_t ty[3] = {EARTH_CY-15, EARTH_CY+10, EARTH_CY-22};
    uint32_t delays[3] = {0, 500, 950};

    for (int i = 0; i < 3; i++) {
        missiles[i].sx = lx[i];  missiles[i].sy = ly[i];
        missiles[i].ex = tx[i];  missiles[i].ey = ty[i];
        missiles[i].step = 0;
        missiles[i].launchDelay = delays[i];
        missiles[i].active = false;
        missiles[i].done   = false;
        for (int j = 0; j < MTRAIL; j++) {
            missiles[i].trailX[j] = lx[i];
            missiles[i].trailY[j] = ly[i];
        }
        booms[i].cx = tx[i]; booms[i].cy = ty[i];
        booms[i].r  = 0;     booms[i].maxR = 18;  booms[i].active = false;
    }
    for (int i = 0; i < MAX_SMOKE; i++) smokeP[i].active = false;
}

static void animMissiles(uint32_t elapsed) {
    static const uint16_t trailCol[MTRAIL] = {COLOR_WHITE, COLOR_YELLOW, COLOR_ORANGE, COLOR_BLACK};
    for (int i = 0; i < 3; i++) {
        if (missiles[i].done) continue;
        if (elapsed < missiles[i].launchDelay) continue;
        missiles[i].active = true;

        // Erase oldest trail pixel
        lcd.drawPixel(missiles[i].trailX[MTRAIL-1], missiles[i].trailY[MTRAIL-1], COLOR_BLACK);
        // Shift trail history
        for (int j = MTRAIL-1; j > 0; j--) {
            missiles[i].trailX[j] = missiles[i].trailX[j-1];
            missiles[i].trailY[j] = missiles[i].trailY[j-1];
        }
        // Interpolate new head position
        int s = missiles[i].step;
        int16_t nx = missiles[i].sx + (int16_t)((long)(missiles[i].ex - missiles[i].sx) * s / MISSILE_STEPS);
        int16_t ny = missiles[i].sy + (int16_t)((long)(missiles[i].ey - missiles[i].sy) * s / MISSILE_STEPS);
        missiles[i].trailX[0] = nx;
        missiles[i].trailY[0] = ny;

        // Draw trail (bright head, fading tail)
        lcd.fillRect(nx-1, ny-1, 3, 3, trailCol[0]);
        for (int j = 1; j < MTRAIL-1; j++)
            lcd.drawPixel(missiles[i].trailX[j], missiles[i].trailY[j], trailCol[j]);

        missiles[i].step++;
        if (missiles[i].step >= MISSILE_STEPS) {
            // Impact — erase trail and trigger explosion
            for (int j = 0; j < MTRAIL; j++)
                lcd.drawPixel(missiles[i].trailX[j], missiles[i].trailY[j], COLOR_BLACK);
            missiles[i].done   = true;
            booms[i].active    = true;
            booms[i].r         = 1;
        }
    }
}

static void animExplosions() {
    for (int i = 0; i < 3; i++) {
        if (!booms[i].active) continue;
        uint16_t c = COLOR_WHITE;
        if (booms[i].r >  4) c = COLOR_YELLOW;
        if (booms[i].r >  8) c = COLOR_ORANGE;
        if (booms[i].r > 12) c = 0xA000;  // dark red
        cgDrawCircle(booms[i].cx, booms[i].cy, booms[i].r, c);
        booms[i].r++;
        if (booms[i].r > booms[i].maxR) booms[i].active = false;
    }
}

static void animBurnEarth(uint32_t elapsed) {
    if (elapsed < 2400) return;  // wait until all missiles have hit
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
        cgDrawCircle(EARTH_CX, EARTH_CY, 32, 0x4208);  // charred dark-gray rim
    }
}

static void animSmoke(uint32_t elapsed) {
    if (elapsed < 2800) return;
    uint32_t now = millis();
    if (now - smokeTickMs < 160) return;
    smokeTickMs = now;
    // Spawn a new particle in the first free slot
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
    // Rise all active smoke particles
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

// Master Phase 2 coordinator — called every frame
static void updateDestroyAnim() {
    initDestroyAnim();
    uint32_t now     = millis();
    uint32_t elapsed = now - destroyStartMs;
    if (now - destroyTickMs < 55) return;   // ~18 fps cap
    destroyTickMs = now;

    animMissiles(elapsed);
    animExplosions();
    animBurnEarth(elapsed);
    animSmoke(elapsed);

    // Auto-advance to Phase 3 (lore text) after 6.2 s
    if (elapsed >= 6200) {
        cgPhase     = 3;
        cgStartTime = millis();
        cgDirty     = true;
    }
}

// ------------------------------------------------------------------
// Phase 1 full draw: space invasion scene
static void drawInvasionScene() {
    lcd.fillScreen(COLOR_BLACK);
    drawStars();
    drawEarth();
    drawShip();
    drawBeams();
    // Title
    lcd.drawText(12, 10,  "O U T P O S T", COLOR_GOLD,   COLOR_BLACK, 1);
    lcd.drawText(20, 22,  "THE  ARRIVAL",  COLOR_RED,    COLOR_BLACK, 1);
    // Subtitle hint
    lcd.drawText(22, 222, "Press [A] to continue", COLOR_GRAY, COLOR_BLACK, 1);
}

// ------------------------------------------------------------------
// Phase 1 full draw: story text panel
static void drawTextScene() {
    lcd.fillScreen(COLOR_BLACK);
    drawStars();

    // Large panel
    lcd.fillRect(10, 30, 220, 175, COLOR_BG_PANEL);
    lcd.drawRect(10, 30, 220, 175, COLOR_GOLD);
    lcd.drawRect(11, 31, 218, 173, 0x4208);   // inner shadow

    // Title line
    lcd.drawText(30, 42, "-- TRANSMISSION LOG --", COLOR_GOLD, COLOR_BG_PANEL, 1);
    cgDrawLine(12, 53, 228, 53, COLOR_GOLD);

    // Story text
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

    // Religious/mystic symbol at bottom
    cgFillCircle(120, 215, 10, 0x4010);
    cgDrawCircle(120, 215, 10, COLOR_MAGENTA);
    cgFillCircle(120, 215,  4, COLOR_MAGENTA);
    cgFillCircle(120, 215,  2, COLOR_YELLOW);
    cgDrawLine(120, 203, 120, 227, COLOR_MAGENTA);
    cgDrawLine(108, 215, 132, 215, COLOR_MAGENTA);

    lcd.drawText(30, 232, "Press [A] to continue", COLOR_GRAY, COLOR_BLACK, 1);
}

// ------------------------------------------------------------------
void cgDraw() {
    if (cgPhase == 0) {
        drawStoryInit();          // clear canvas; typewriter fills it via cgDrawUpdate()
    } else if (cgPhase == 1) {
        drawInvasionScene();
    } else if (cgPhase == 2) {
        drawInvasionScene();      // backdrop; destruction animates over it each frame
        destroyInited = false;    // ensure initDestroyAnim() runs on first update tick
    } else {
        drawTextScene();
    }
}

// ------------------------------------------------------------------
bool cgUpdate() {
    static bool prevBtnA = false;
    static bool prevBtnBlue = false;

    if (cgStartTime == 0) {
        cgStartTime = millis();
        randomSeed(micros());
        prevBtnBlue = digitalRead(PC13);
    }

    // Check Blue User Button (PC13) to stop music at any time. Wait for signal stability
    bool curBtnBlue = digitalRead(PC13);
    // Note: STM32 Nucleo L476RG USER_BTN is usually active HIGH (B1 to PC13)
    if (curBtnBlue == HIGH && prevBtnBlue == LOW) {
        Serial.println("STOP_MUSIC");
    }
    prevBtnBlue = curBtnBlue;

    bool curBtnA    = (digitalRead(BTN_A) == LOW);
    bool btnPressed = curBtnA && !prevBtnA;
    prevBtnA = curBtnA;

    if (!btnPressed) return false;

    if (cgPhase == 0) {
        cgPhase     = 1;       // A → skip story text, go to invasion scene
        cgStartTime = millis();
        cgDirty     = true;
        return false;
    }
    if (cgPhase == 1) {
        cgPhase     = 2;       // A → skip static scene, start destruction anim
        cgStartTime = millis();
        cgDirty     = true;
        return false;
    }
    if (cgPhase == 2) {
        cgPhase     = 3;       // A → skip destruction, go to lore text
        cgStartTime = millis();
        cgDirty     = true;
        return false;
    }
    // ------------------------------------------------------------------
    // Send a signal over Serial to the PC to stop playing the music
    Serial.println("STOP_MUSIC");

    return true;
}