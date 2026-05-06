#ifndef INVENTORY_DATA_H
#define INVENTORY_DATA_H

#include <Arduino.h>

#define ITEM_SEED          0
#define ITEM_FOOD          1
#define ITEM_METAL         2
#define ITEM_ARMOR         3
#define ITEM_RELIC         4
#define ITEM_MEDKIT        5
#define ITEM_TYPES_TOTAL   6

struct Inventory {
  int seedCount;
  int foodCount;
  int metalCount;
  int armorCount;
  int relicCount;
};

void resetInventory(struct Inventory* inv);
void addItemToInventory(struct Inventory* inv, int item, int amount);

#endif