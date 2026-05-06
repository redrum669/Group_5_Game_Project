#ifndef MAP_DROP_SYSTEM_H
#define MAP_DROP_SYSTEM_H

#include <Arduino.h>
#include "inventory.h"
#include "config.h"

#define MAX_DROP_ENTITIES  24

struct MapDrop {
  int tileX;
  int tileY;
  int itemCategory;
  bool enabled;
};

extern struct MapDrop dropList[MAX_DROP_ENTITIES];

void initDropsOnMap(uint8_t map[MAP_GRID_ROWS][MAP_GRID_COLS]);
void spawnDropsByArea(uint8_t map[MAP_GRID_ROWS][MAP_GRID_COLS], int areaID);
int checkPickup(int x, int y, struct Inventory* inv);
bool dropPresentAt(int x, int y);
int getDropItemType(int x, int y);
void spawnDrop(int x, int y, int item);
bool spawnDropRandom(int item, uint8_t map[MAP_GRID_ROWS][MAP_GRID_COLS], int* rx, int* ry);

#endif