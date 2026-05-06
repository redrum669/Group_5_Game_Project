#ifndef CHARACTER_RENDER_H
#define CHARACTER_RENDER_H

#include <Arduino.h>
#include "config.h"

// character sprite definitions
#define CHAR_DEFAULT_FRAME  0
#define CHAR_ATTACK_FRAME   1
#define CHAR_HURT_FRAME     2
#define SPRITE_SCALE_DEFAULT 2

// player render shadow offset
#define SHADOW_DRAW_OFFSET  4

void drawPlayerCharacter(int pixelX, int pixelY, int wType);
void drawEnemySprite(int eType, int px, int py);
void drawSpriteFrame(int x, int y, int frame, int color);
void drawEntityShadow(int xPos, int yPos);

#endif