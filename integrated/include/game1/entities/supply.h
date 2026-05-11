#ifndef SUPPLY_H
#define SUPPLY_H

#include <Arduino.h>

void supplyDrawGround(int px, int py, int tx, int ty);
void supplyDrawDecor(int px, int py, int tx, int ty);
void supplyDrawBlocker(int px, int py, int tx, int ty);
bool supplyNearMerchant(int playerX, int playerY);
bool supplyNearWarrior(int playerX, int playerY);
bool supplyNearCivilian(int playerX, int playerY);
bool supplyNearWizard(int playerX, int playerY);
bool supplyIsSolidEntity(int tileX, int tileY);

#endif
