#ifndef ALTAR_STRUCTS_H
#define ALTAR_STRUCTS_H

#include <Arduino.h>

// altar interaction state machine
#define ALTAR_INACTIVE     0
#define ALTAR_ACTIVE       1
#define ALTAR_COMPLETE     2

// altar render and trigger constants
#define ALTAR_UI_OFFSET_Y  40
#define ALTAR_TRIGGER_RANGE 2

struct GameAltar {
  int state;
  int posTileX;
  int posTileY;
  bool playerInRange;
  uint32_t triggerTimer;
};

extern struct GameAltar altarEntity;

void initAltar(void);
void updateAltarState(int pX, int pY);
void drawAltarStructure(void);
bool checkAltarCompletion(void);

#endif