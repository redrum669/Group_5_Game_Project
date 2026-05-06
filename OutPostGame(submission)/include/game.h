#ifndef GAME_CORE_H
#define GAME_CORE_H

#include <Arduino.h>
#include "inventory.h"
#include "weapon.h"

enum GameMode {
  MODE_BOOT,
  MODE_CUT1,
  MODE_MENU,
  MODE_EXPLORE,
  MODE_WORLDMAP,
  MODE_CUT2
};

struct PlayerData {
  int tileX;
  int tileY;
  int8_t moveDirX;
  int8_t moveDirY;
  uint8_t moveStep;
  bool moving;
  int lastDrawX;
  int lastDrawY;
  int hp;
  int hunger;
  int armor;
  struct Inventory inventory;
  struct Weapon equip;
  int pixelX;
  int pixelY;
  int velocityY;
  bool onGround;
  int cameraOffsetX;
};

extern enum GameMode currentGameMode;
extern struct PlayerData player;

void initializeGame(void);
void tickGameLogic(void);
void renderGameFrame(void);

#endif