#ifndef GAME_H
#define GAME_H

#include <Arduino.h>
#include "game1/systems/inventory.h"
#include "game1/entities/weapon.h"

enum GameState {
    STATE_BOOT,
    STATE_CG,
    STATE_MENU,
    STATE_EXPLORE,
    STATE_WORLDMAP,
    STATE_CG2
};

struct Player {
    int       tileX;
    int       tileY;
    // Smooth-movement interpolation state.
    // tileX/tileY stay at the START tile until the move completes; target tile is (tileX+moveDirX, tileY+moveDirY).
    int8_t    moveDirX;         // -1 / 0 / +1
    int8_t    moveDirY;         // -1 / 0 / +1
    uint8_t   moveProgress;     // 0..TILE_SIZE pixels into current step
    bool      isMoving;
    int       lastDrawnPixelX;  // previous-frame draw position (absolute screen coords)
    int       lastDrawnPixelY;
    int       hp;
    int       hunger;
    int       armor;    // armor stat (0-100)
    Inventory inv;
    Weapon    weapon;   // currently equipped weapon

    // Platformer physics state (combat areas)
    int       pixelX;           // Absolute pixel X (replaces tileX * TILE_SIZE logic)
    int       pixelY;           // Absolute pixel Y
    int       velY;             // Y velocity (for gravity/jumping)
    bool      grounded;         // Is player standing on ground?
    int       cameraX;          // Camera offset (pixels)
};

void gameInit();
void gameUpdate();
void gameRender();

// Unified-app hooks
void gameSetStartState(int mode); // 0 = boot/cg, 1 = worldmap directly
bool gameIsRequestingReturnToHost();
void gameClearReturnRequest();

#endif
