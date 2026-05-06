#include "worldmap.h"
#include "st7789.h"
#include "input.h"
#include <string.h>
#include <stdlib.h>

// Color definitions for world map UI
#define WM_BG      0x20E2
#define WM_CRACK   0x1082
#define WM_DUST    0x3163
#define WM_PATH    0x41A4
#define WM_ICON    0x2943
#define WM_BORDER  0x4A69
#define WM_SEL     0xFD20
#define WM_WALL    0x528A
#define WM_RUST    0xA965
#define WM_BONE    0xBDF7
#define WM_GLOW    0xF81F
#define WM_TEXT    0xC618
#define WM_TITLE   0xFFE0
#define WM_HINT    0x7BEF

// Default empty map layout
static const uint8_t defaultMap[MAP_ROWS][MAP_COLS] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// Area map data
static const uint8_t ashrootFarmMap[MAP_ROWS][MAP_COLS] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,2,0,0,0,0},
    {0,0,0,0,0,2,0,0,0,0,0,9,0,0},
    {0,2,0,0,0,0,0,0,2,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,2,0,0,0},
    {0,0,0,2,0,0,0,0,0,0,0,0,2,0},
    {0,0,0,0,0,0,0,2,0,0,0,0,0,0},
    {0,0,2,0,0,0,0,0,0,2,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,2,0,0},
    {0,0,0,0,2,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static const uint8_t dawnSupplyMap[MAP_ROWS][MAP_COLS] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,9,0,0},
    {0,0,2,0,1,1,1,1,1,1,0,2,0,0},
    {0,0,0,0,1,1,1,1,1,1,0,0,0,0},
    {0,0,0,0,1,0,0,0,0,1,0,0,0,0},
    {0,0,2,0,2,0,2,0,2,0,2,0,0,0},
    {0,2,0,0,1,1,0,0,1,1,0,2,2,0},
    {0,0,0,0,1,1,0,0,1,1,0,2,0,0},
    {0,1,1,2,0,0,0,0,0,0,2,1,1,0},
    {0,1,1,0,0,0,0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static const uint8_t rustforgeMap[MAP_ROWS][MAP_COLS] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,9,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

static const uint8_t ashveilMap[MAP_ROWS][MAP_COLS] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

static const uint8_t lostExpanseMap[MAP_ROWS][MAP_COLS] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

static const uint8_t eclipseBaseMap[MAP_ROWS][MAP_COLS] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

static const uint8_t relicAltarMap[MAP_ROWS][MAP_COLS] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,2,0,0,0,0,2,0,9,0,0},
    {0,0,0,0,0,0,2,0,0,0,2,0,0,0},
    {0,0,2,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,2,1,1,1,1,2,0,2,0,0},
    {0,0,0,2,0,1,1,1,1,0,2,0,0,0},
    {0,0,0,0,0,0,1,1,0,2,0,0,0,0},
    {0,0,0,0,0,0,0,0,2,0,0,2,0,0},
    {0,0,0,0,0,2,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

// Storage for all area maps
static uint8_t areaMaps[AREA_COUNT][MAP_ROWS][MAP_COLS];
// Currently selected area on world map
static int selectedArea = AREA_HOME;

// Display names for each area
static const char* areaNames[AREA_COUNT] = {
    "Ashroot Farm",
    "Dawn Supply",
    "Rustforge Ruins",
    "Ashveil Lands",
    "Lost Expanse",
    "Eclipse Base",
    "Relic Altar"
};

// Screen coordinates for area icons
static const int areaX[AREA_COUNT] = { 15, 155,  15, 155, 155,  90,  15};
static const int areaY[AREA_COUNT] = { 35,  35, 103, 103, 188, 148, 188};

// Connected path pairs between areas
#define PATH_COUNT 8
static const int8_t pathPairs[PATH_COUNT][2] = {
    {AREA_HOME,       AREA_TRADE},
    {AREA_HOME,       AREA_INDUSTRIAL},
    {AREA_TRADE,      AREA_FARMLAND},
    {AREA_INDUSTRIAL, AREA_FARMLAND},
    {AREA_INDUSTRIAL, AREA_BASE},
    {AREA_FARMLAND,   AREA_BASE},
    {AREA_BASE,       AREA_ALTAR},
    {AREA_BASE,       AREA_RANDOM},
};

/**
 * @brief Initialize world map data and load all area layouts
 */
void worldmapInit(void) {
    // Load default empty map to all areas
    for (int a = 0; a < AREA_COUNT; a++) {
        memcpy(areaMaps[a], defaultMap, sizeof(defaultMap));
    }

    // Load custom map data for each area
    memcpy(areaMaps[AREA_HOME],       ashrootFarmMap,  sizeof(ashrootFarmMap));
    memcpy(areaMaps[AREA_TRADE],      dawnSupplyMap,   sizeof(dawnSupplyMap));
    memcpy(areaMaps[AREA_INDUSTRIAL], rustforgeMap,    sizeof(rustforgeMap));
    memcpy(areaMaps[AREA_FARMLAND],   ashveilMap,      sizeof(ashveilMap));
    memcpy(areaMaps[AREA_RANDOM],     lostExpanseMap,  sizeof(lostExpanseMap));
    memcpy(areaMaps[AREA_BASE],       eclipseBaseMap,  sizeof(eclipseBaseMap));
    memcpy(areaMaps[AREA_ALTAR],      relicAltarMap,   sizeof(relicAltarMap));
}

/**
 * @brief Get currently selected area index
 * @return Selected area ID
 */
int worldmapGetSelectedArea(void) {
    return selectedArea;
}

/**
 * @brief Check if area uses platformer physics
 * @param areaId Target area to check
 * @return true if platformer, false if top-down
 */
bool worldmapIsPlatformerArea(int areaId) {
    return (areaId == AREA_INDUSTRIAL ||
            areaId == AREA_FARMLAND ||
            areaId == AREA_RANDOM ||
            areaId == AREA_BASE);
}

/**
 * @brief Draw wasteland background with cracks and dust details
 */
static void drawWastelandBg(void) {
    lcd.fillScreen(WM_BG);

    // Draw random cracks
    for (int i = 0; i < 50; i++) {
        int px = random(0, 220);
        int py = random(18, 224);
        lcd.drawFastHLine(px, py, random(5, 22), WM_CRACK);
    }
    for (int i = 0; i < 18; i++) {
        int px = random(0, 240);
        int py = random(18, 218);
        lcd.drawFastVLine(px, py, random(3, 9), WM_CRACK);
    }

    // Draw dust particles
    for (int i = 0; i < 65; i++) {
        lcd.drawPixel(random(0, 240), random(18, 224), WM_DUST);
    }
}

/**
 * @brief Draw connecting paths between area icons
 */
static void drawRoads(void) {
    for (int p = 0; p < PATH_COUNT; p++) {
        int a = pathPairs[p][0];
        int b = pathPairs[p][1];

        int x1 = areaX[a] + 12;
        int y1 = areaY[a] + 12;
        int x2 = areaX[b] + 12;
        int y2 = areaY[b] + 12;

        int dx = x2 - x1;
        int dy = y2 - y1;
        int steps = (abs(dx) > abs(dy)) ? abs(dx) : abs(dy);

        if (steps == 0) continue;

        // Draw line between two areas
        for (int s = 0; s <= steps; s++) {
            int px = x1 + (dx * s) / steps;
            int py = y1 + (dy * s) / steps;
            lcd.drawPixel(px, py, WM_PATH);

            // Make path slightly thicker
            if (abs(dx) >= abs(dy))
                lcd.drawPixel(px, py + 1, WM_PATH);
            else
                lcd.drawPixel(px + 1, py, WM_PATH);
        }
    }
}

/**
 * @brief Draw icon for a single area on world map
 * @param x Screen X position
 * @param y Screen Y position
 * @param area Area type to draw
 * @param sel Whether this area is currently selected
 */
static void drawAreaIcon(int x, int y, int area, bool sel) {
    // Draw icon background and border
    lcd.fillRect(x, y, 24, 24, WM_ICON);
    lcd.drawRect(x, y, 24, 24, sel ? WM_SEL : WM_BORDER);
    if (sel) lcd.drawRect(x + 1, y + 1, 22, 22, WM_SEL);

    int ix = x + 4;
    int iy = y + 4;

    // Draw unique symbol for each area
    switch (area) {
        case AREA_HOME:
            lcd.fillRect(ix + 6, iy + 4, 3, 9, 0x6A04);
            lcd.fillRect(ix + 3, iy + 2, 9, 3, 0x7387);
            lcd.drawFastHLine(ix + 2, iy + 12, 12, 0x5202);
            break;

        case AREA_TRADE:
            lcd.fillRect(ix + 2, iy + 6, 12, 7, WM_WALL);
            lcd.fillRect(ix + 4, iy + 3, 8, 3, COLOR_RED);
            lcd.drawFastVLine(ix + 8, iy, 6, COLOR_YELLOW);
            break;

        case AREA_INDUSTRIAL:
            lcd.fillRect(ix + 1, iy + 8, 14, 6, WM_WALL);
            lcd.fillRect(ix + 10, iy + 1, 3, 9, 0x632B);
            lcd.fillRect(ix + 10, iy, 3, 2, COLOR_ORANGE);
            break;

        case AREA_FARMLAND:
            lcd.drawFastVLine(ix + 7, iy + 3, 9, WM_RUST);
            lcd.drawFastHLine(ix + 2, iy + 11, 10, 0x4228);
            lcd.drawPixel(ix + 4, iy + 5, WM_DUST);
            lcd.drawPixel(ix + 10, iy + 7, WM_DUST);
            break;

        case AREA_RANDOM:
            lcd.fillRect(ix + 3, iy + 4, 10, 5, WM_BONE);
            lcd.fillRect(ix + 5, iy + 9, 6, 4, WM_BONE);
            lcd.fillRect(ix + 5, iy + 5, 2, 2, WM_ICON);
            lcd.fillRect(ix + 9, iy + 5, 2, 2, WM_ICON);
            break;

        case AREA_BASE:
            lcd.fillRect(ix + 7, iy + 2, 2, 10, WM_WALL);
            lcd.fillRect(ix + 3, iy + 4, 10, 2, 0x5B1C);
            lcd.fillRect(ix + 6, iy, 4, 2, COLOR_RED);
            break;

        case AREA_ALTAR:
            lcd.fillRect(ix + 5, iy + 3, 6, 9, 0x6B2D);
            lcd.fillRect(ix + 3, iy + 12, 10, 2, 0x4A49);
            lcd.fillRect(ix + 6, iy + 1, 4, 2, WM_GLOW);
            break;
    }
}

/**
 * @brief Full render of world map UI
 */
void worldmapDraw(void) {
    drawWastelandBg();
    drawRoads();

    // Draw all area icons and labels
    for (int i = 0; i < AREA_COUNT; i++) {
        drawAreaIcon(areaX[i], areaY[i], i, (i == selectedArea));
        lcd.drawText(areaX[i], areaY[i] + 26, areaNames[i], WM_TEXT, WM_BG, 1);
    }

    // Draw title and control hints
    lcd.drawText(55, 6, "WASTELAND MAP", WM_TITLE, WM_BG, 1);
    lcd.drawText(10, 228, "<>:SWITCH", WM_HINT, WM_BG, 1);
    lcd.drawText(150, 228, "A:ENTER", WM_HINT, WM_BG, 1);
}

/**
 * @brief Update only changed icons during area selection
 * @param oldArea Previous selection
 * @param newArea New selection
 */
static void worldmapDrawIconUpdate(int oldArea, int newArea) {
    drawAreaIcon(areaX[oldArea], areaY[oldArea], oldArea, false);
    lcd.drawText(areaX[oldArea], areaY[oldArea] + 26, areaNames[oldArea], WM_TEXT, WM_BG, 1);

    drawAreaIcon(areaX[newArea], areaY[newArea], newArea, true);
    lcd.drawText(areaX[newArea], areaY[newArea] + 26, areaNames[newArea], WM_TEXT, WM_BG, 1);
}

/**
 * @brief Update world map input (left/right to switch areas)
 * @return true if exit requested, false otherwise
 */
bool worldmapUpdate(void) {
    static bool prevLeft = false;
    static bool prevRight = false;

    bool curLeft  = isLeftPressed();
    bool curRight = isRightPressed();

    // Left button: cycle to previous area
    if (curLeft && !prevLeft) {
        int oldArea = selectedArea;
        selectedArea = (selectedArea - 1 + AREA_COUNT) % AREA_COUNT;
        worldmapDrawIconUpdate(oldArea, selectedArea);
    }

    // Right button: cycle to next area
    if (curRight && !prevRight) {
        int oldArea = selectedArea;
        selectedArea = (selectedArea + 1) % AREA_COUNT;
        worldmapDrawIconUpdate(oldArea, selectedArea);
    }

    prevLeft  = curLeft;
    prevRight = curRight;

    return false;
}

/**
 * @brief Load currently selected area's tile map
 * @param destMap Destination array to store map data
 * @return true if load succeeded
 */
bool worldmapLoadCurrentAreaMap(uint8_t destMap[MAP_ROWS][MAP_COLS]) {
    if (selectedArea >= 0 && selectedArea < AREA_COUNT) {
        memcpy(destMap, areaMaps[selectedArea], sizeof(areaMaps[selectedArea]));
        return true;
    }
    return false;
}

/**
 * @brief Set safe initial player position for current area
 * @param mapData Level tile data
 * @param playerX Output X tile position
 * @param playerY Output Y tile position
 */
void worldmapResetPlayerPosition(uint8_t mapData[MAP_ROWS][MAP_COLS], int* playerX, int* playerY) {
    *playerX = 2;

    if (worldmapIsPlatformerArea(selectedArea)) {
        // Find ground level in platformer mode
        for (int y = 0; y < MAP_ROWS; y++) {
            if (mapData[y][*playerX] == 1) {
                *playerY = (y - 1 > 0) ? y - 1 : 0;
                return;
            }
        }
        *playerY = 2;
    } else {
        // Default position for top-down areas
        *playerY = 2;
    }
}