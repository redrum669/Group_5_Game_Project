#ifndef INVENTORY_H
#define INVENTORY_H

#include <Arduino.h>

// =====================================================
// Item type indices — used with inventoryAdd()
// =====================================================
#define INV_SEEDS        0
#define INV_FOOD         1
#define INV_METAL        2
#define INV_ARMOR        3
#define INV_ALIEN_RELICS 4
#define INV_MEDKIT       5   // auto-used on pickup — not stored in inventory
#define INV_ITEM_COUNT   6

// =====================================================
// Inventory data
// =====================================================
struct Inventory {
    int seeds;
    int food;
    int metal;
    int armor;        // Armor pieces (separate from player armor stat)
    int alienRelics;
};

// =====================================================
// Functions
// =====================================================
void inventoryInit(Inventory* inv);
void inventoryAdd(Inventory* inv, int itemType, int amount);

#endif

