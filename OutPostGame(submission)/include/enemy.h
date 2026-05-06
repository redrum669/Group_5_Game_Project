#ifndef ENEMY_UNIT_H
#define ENEMY_UNIT_H

#include <Arduino.h>
#include "config.h"

#define ENEMY_TYPE_KNIFE    0
#define ENEMY_TYPE_LASER    1
#define ENEMY_TYPE_HEAVY    2
#define MAX_ENEMY_UNITS     8

struct EnemyUnit {
  int tileX;
  int tileY;
  int health;
  bool active;
  uint32_t lastMoveMs;
  uint32_t lastAttackMs;
  int type;
  bool markedForCleanup;
};

extern struct EnemyUnit enemyList[MAX_ENEMY_UNITS];

void spawnEnemiesForArea(uint8_t map[MAP_GRID_ROWS][MAP_GRID_COLS], int area);
void updateAllEnemies(int pX, int pY, uint8_t map[MAP_GRID_ROWS][MAP_GRID_COLS]);
void cleanupDeadEnemies(void);
bool isEnemyAt(int x, int y);
int enemyCountInArea(void);

#endif