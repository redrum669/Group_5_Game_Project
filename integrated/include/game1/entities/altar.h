#ifndef ALTAR_H
#define ALTAR_H

#include <Arduino.h>

void altarDrawGround(int px, int py, int tx, int ty);
void altarDrawDecor(int px, int py, int tx, int ty);
void altarDrawBlocker(int px, int py, int tx, int ty);
bool altarNearPriest(int playerX, int playerY);
bool altarNearSkull(int playerX, int playerY);
bool altarNearOffering(int playerX, int playerY);

#endif
