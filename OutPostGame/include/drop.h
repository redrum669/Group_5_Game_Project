#ifndef DROP_H
#define DROP_H

#include <Arduino.h>
#include "inventory.h"
#include "config.h"

// Maximum number of drop slots on the map
#define MAX_DROPS 24

// =====================================================
// A single dropped item on the map
// itemType uses INV_* constants from inventory.h
// =====================================================
struct Drop {
    int  tileX;
    int  tileY;
    int  itemType;   // INV_SEEDS / INV_FOOD / INV_METAL / INV_ALIEN_RELICS
    bool active;
};

// Global drop array (defined in drop.cpp)
extern Drop drops[MAX_DROPS];

// =====================================================
// Functions
// =====================================================

// Place drops randomly on empty map tiles (generic, equal distribution)
void dropInit(uint8_t mapData[MAP_ROWS][MAP_COLS]);

// Place drops with an area-specific loot table (areaId = AREA_* constant)
void dropInitForArea(uint8_t mapData[MAP_ROWS][MAP_COLS], int areaId);

// Check if player stepped on a drop; if so, pick it up.
// Returns the itemType picked up, or -1 if nothing was there.
// INV_MEDKIT drops are NOT added to inventory (caller handles HP restore).
int dropPickup(int tileX, int tileY, Inventory* inv);

// Returns true if an active drop exists at the given tile
bool dropExistsAt(int tileX, int tileY);

// Returns itemType of drop at tile, or -1 if none
int dropTypeAt(int tileX, int tileY);

// Add a drop at the given tile (used when enemy dies)
void dropAdd(int tileX, int tileY, int itemType);

// Place itemType at a random empty tile on the map.
// Writes the chosen coordinates to *outX / *outY (if not null).
// Returns true if a slot was found, false if the map is too full.
bool dropAddRandom(int itemType, uint8_t mapData[MAP_ROWS][MAP_COLS], int* outX, int* outY);

#endif

