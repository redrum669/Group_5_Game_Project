#include "inventory.h"

// Reset all inventory counts to zero
void inventoryInit(Inventory* inv) {
    inv->seeds       = 0;
    inv->food        = 0;
    inv->metal       = 0;
    inv->armor       = 0;
    inv->alienRelics = 0;
}

// Add (or subtract) items by type; clamps to >= 0
void inventoryAdd(Inventory* inv, int itemType, int amount) {
    switch (itemType) {
        case INV_SEEDS:        inv->seeds       += amount; if (inv->seeds       < 0) inv->seeds       = 0; break;
        case INV_FOOD:         inv->food         += amount; if (inv->food        < 0) inv->food        = 0; break;
        case INV_METAL:        inv->metal        += amount; if (inv->metal       < 0) inv->metal       = 0; break;
        case INV_ARMOR:        inv->armor        += amount; if (inv->armor       < 0) inv->armor       = 0; break;
        case INV_ALIEN_RELICS: inv->alienRelics  += amount; if (inv->alienRelics < 0) inv->alienRelics = 0; break;
        default: break;
    }
}

