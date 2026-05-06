#ifndef WORLD_MAP_H
#define WORLD_MAP_H

#include <Arduino.h>
#include "config.h"

#define AREA_HOME          0
#define AREA_TRADE         1
#define AREA_INDUSTRIAL    2
#define AREA_FARM          3
#define AREA_RANDOM        4
#define AREA_BASE          5
#define AREA_ALTAR         6
#define TOTAL_AREAS        7

struct WorldMap {
  int currentArea;
  bool areaCleared[TOTAL_AREAS];
  bool mapUnlocked[TOTAL_AREAS];
};

extern struct WorldMap worldMap;

void initWorldMap(void);
void loadAreaData(int area, uint8_t map[MAP_GRID_ROWS][MAP_GRID_COLS]);
void moveToArea(int newArea);
bool isAreaClear(int area);
void markAreaClear(int area);

#endif