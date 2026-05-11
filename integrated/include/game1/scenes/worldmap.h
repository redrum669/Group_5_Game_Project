#ifndef WORLDMAP_H
#define WORLDMAP_H

#include <Arduino.h>
#include "config.h"

// Area IDs
#define AREA_HOME       0
#define AREA_TRADE      1
#define AREA_INDUSTRIAL 2
#define AREA_FARMLAND   3
#define AREA_RANDOM     4
#define AREA_BASE       5
#define AREA_ALTAR      6
#define AREA_COUNT      7

// Initialize world map data (load maps etc.)
void worldmapInit();

// Update world map logic (handle input, selection). Returns true if selection changed.
bool worldmapUpdate();

// Draw the world map (full screen)
void worldmapDraw();

// Returns the currently selected area ID (AREA_* constant)
int worldmapGetSelectedArea();

// Returns true if the area is a platformer (combat) area
bool worldmapIsPlatformerArea(int areaId);

// Load the currently selected area's map into the provided buffer
bool worldmapLoadCurrentAreaMap(uint8_t destMap[MAP_ROWS][MAP_COLS]);

// Reset player position for the current map (to a safe starting tile)
void worldmapResetPlayerPosition(uint8_t mapData[MAP_ROWS][MAP_COLS], int* playerX, int* playerY);

#endif
