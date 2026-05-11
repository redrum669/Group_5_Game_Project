#ifndef ENEMY_H
#define ENEMY_H

#include <Arduino.h>
#include "config.h"
#include "game1/core/game.h"

// Enemy type IDs
#define ENEMY_KNIFE  0
#define ENEMY_LASER  1
#define ENEMY_HEAVY  2

// Enemy constants
#define MAX_ENEMIES      10

// Knife alien
#define KNIFE_HP         50
#define KNIFE_MOVE_MS    700
#define KNIFE_MOVE_PX    1    // pixels per frame while interpolating (16 px = 8 frames â‰?130 ms)
#define KNIFE_ATK_MS     900
#define KNIFE_DMG        12

// Laser alien
#define LASER_HP         30
#define LASER_MOVE_MS    750
#define LASER_MOVE_PX    1
#define LASER_ATK_MS     1400
#define LASER_DMG        10
#define LASER_RANGE      6

// Heavy alien
#define HEAVY_HP         120
#define HEAVY_MOVE_MS    1400
#define HEAVY_MOVE_PX    1    // slow brute: 16 frames â‰?260 ms / tile
#define HEAVY_ATK_MS     1200
#define HEAVY_DMG        25

// Player smooth-movement speed
#define PLAYER_MOVE_PX   3    // ~6 frames / tile â‰?100 ms

// Legacy alias
#define PLAYER_ATK_DMG   25

struct Enemy {
    int      tileX;
    int      tileY;
    int      prevX;
    int      prevY;

    // Platformer physics state (combat areas)
    int      pixelX;
    int      pixelY;
    int      velY;
    bool     grounded;

    int      hp;
    bool     active;
    bool     moved;
    uint32_t lastMove;
    uint32_t lastAtk;
    int      type;
    bool     laserFired;
    int      laserEndX;
    int      laserEndY;
    bool     alerted;
    // Smooth-movement interpolation (same model as Player).
    // tileX/tileY stay at START tile until the step completes.
    int8_t   moveDirX;
    int8_t   moveDirY;
    uint8_t  moveProgress;     // 0..TILE_SIZE
    bool     isMoving;
    int      lastDrawnPixelX;
    int      lastDrawnPixelY;
    // Set when active transitions to false. The renderer consumes this to
    // erase the enemy's full 16x16 sprite rect at its last interpolated
    // position (may span up to 4 tiles) so mid-step deaths leave no ghost.
    bool     pendingDeathClear;
};

extern Enemy enemies[MAX_ENEMIES];

void enemyInit(uint8_t mapData[MAP_ROWS][MAP_COLS], int playerX, int playerY, int areaId);
bool enemyUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player);
bool playerAttackEnemy(int playerX, int playerY);
bool enemyAt(int tileX, int tileY);
bool enemyAllDead();
void enemyAlertAll();

#endif
