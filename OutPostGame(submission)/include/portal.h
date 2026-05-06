#ifndef PORTAL_LOGIC_H
#define PORTAL_LOGIC_H

#include <Arduino.h>
#include "config.h"

struct MapPortal {
  bool activated;
  int tileX;
  int tileY;
};

extern struct MapPortal levelPortal;

void resetPortal(void);
bool spawnPortalOnMap(uint8_t map[MAP_GRID_ROWS][MAP_GRID_COLS]);
bool isPlayerNearPortal(int x, int y);

#endif