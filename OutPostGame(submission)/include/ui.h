#ifndef UI_RENDER_H
#define UI_RENDER_H

#include <Arduino.h>
#include "game.h"
#include "config.h"

#define UI_BAR_HEIGHT      6
#define UI_PADDING         4
#define UI_TEXT_SMALL      1
#define UI_TEXT_LARGE      2

void drawExploreUI(uint8_t map[MAP_GRID_ROWS][MAP_GRID_COLS], struct PlayerData* p);
void drawPlayerStatus(struct PlayerData* p);
void drawInventoryBar(struct Inventory* inv);
void drawCrosshair(int x, int y);
void drawBulletEffect(uint8_t map[MAP_GRID_ROWS][MAP_GRID_COLS], int sx, int sy, int tx, int ty, int wType);
void drawWorldMapUI(int currentArea);
void drawSimpleTextBox(const char* msg, int yPos);

#endif