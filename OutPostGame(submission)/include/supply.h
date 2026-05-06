#ifndef SUPPLY_STATION_H
#define SUPPLY_STATION_H

#include <Arduino.h>

struct SupplyPoint {
  int x;
  int y;
  bool available;
  bool used;
};

extern struct SupplyPoint supplyStation;

void setupSupplyPoint(int xPos, int yPos);
void drawSupplyStation(void);
bool playerAtSupply(int pX, int pY);
void triggerSupplyRefill(struct PlayerData* p);

#endif