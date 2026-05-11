#ifndef PORTAL_H
#define PORTAL_H

#include <Arduino.h>
#include "config.h"

// =====================================================
// Portal â€?spawns in a map corner after board clear
// =====================================================
struct Portal {
    bool active;
    int  tileX;
    int  tileY;
};

// Global portal state (defined in portal.cpp)
extern Portal portal;

// Reset portal to inactive
void portalReset();

// Try to place the portal near a random map corner on an empty tile.
// Returns true if a valid tile was found.
bool portalSpawn(uint8_t mapData[MAP_ROWS][MAP_COLS]);

// Returns true if the player is within 1 tile of the portal (adjacency + standing on it)
bool portalNearPlayer(int playerX, int playerY);

#endif

