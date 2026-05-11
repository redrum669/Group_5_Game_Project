#include "drop.h"
#include "inventory.h"
#include "worldmap.h"

// Global drop array
Drop drops[MAX_DROPS];

void dropInit(uint8_t mapData[MAP_ROWS][MAP_COLS]) {
    for (int i = 0; i < MAX_DROPS; i++) {
        drops[i].active = false;
    }

    int itemTypes[6] = {
        INV_SEEDS, INV_METAL, INV_ALIEN_RELICS,
        INV_SEEDS, INV_METAL, INV_ALIEN_RELICS
    };
    int totalDrops = 6;

    for (int i = 0; i < totalDrops; i++) {
        int tx, ty;
        bool placed = false;

        for (int attempt = 0; attempt < 200; attempt++) {
            tx = random(1, MAP_COLS - 1);
            ty = random(1, MAP_ROWS - 1);

            if (mapData[ty][tx] == 1) continue;
            if (dropExistsAt(tx, ty)) continue;

            drops[i] = { tx, ty, itemTypes[i], true };
            placed = true;
            break;
        }

        (void)placed;
    }
}

int dropPickup(int tileX, int tileY, Inventory* inv) {
    for (int i = 0; i < MAX_DROPS; i++) {
        if (drops[i].active &&
            drops[i].tileX == tileX &&
            drops[i].tileY == tileY) {
            int type = drops[i].itemType;
            drops[i].active = false;

            if (type != INV_MEDKIT) {
                inventoryAdd(inv, type, 1);
            }
            return type;
        }
    }
    return -1;
}

bool dropExistsAt(int tileX, int tileY) {
    for (int i = 0; i < MAX_DROPS; i++) {
        if (drops[i].active &&
            drops[i].tileX == tileX &&
            drops[i].tileY == tileY) {
            return true;
        }
    }
    return false;
}

void dropAdd(int tileX, int tileY, int itemType) {
    for (int i = 0; i < MAX_DROPS; i++) {
        if (!drops[i].active) {
            drops[i] = { tileX, tileY, itemType, true };
            return;
        }
    }
}

int dropTypeAt(int tileX, int tileY) {
    for (int i = 0; i < MAX_DROPS; i++) {
        if (drops[i].active &&
            drops[i].tileX == tileX &&
            drops[i].tileY == tileY) {
            return drops[i].itemType;
        }
    }
    return -1;
}

bool dropAddRandom(int itemType, uint8_t mapData[MAP_ROWS][MAP_COLS], int* outX, int* outY) {
    bool isPlatformer = worldmapIsPlatformerArea(worldmapGetSelectedArea());
    for (int attempt = 0; attempt < 200; attempt++) {
        int tx = random(1, MAP_COLS - 1);
        int ty = random(1, MAP_ROWS - 1);

        if (mapData[ty][tx] == 1) continue;
        if (dropExistsAt(tx, ty)) continue;

        if (isPlatformer) {
            if (ty >= MAP_ROWS - 1) continue;
            if (mapData[ty + 1][tx] != 1) continue;
        }

        dropAdd(tx, ty, itemType);
        if (outX) *outX = tx;
        if (outY) *outY = ty;
        return true;
    }
    return false;
}

static void placeRandomDropsOfType(uint8_t mapData[MAP_ROWS][MAP_COLS],
                                   int itemType, int count) {
    for (int i = 0; i < count; i++) {
        dropAddRandom(itemType, mapData, nullptr, nullptr);
    }
}

void dropInitForArea(uint8_t mapData[MAP_ROWS][MAP_COLS], int areaId) {
    for (int i = 0; i < MAX_DROPS; i++) {
        drops[i].active = false;
    }

    // Keep these special areas untouched: no random resource scatter here.
    if (areaId == AREA_HOME || areaId == AREA_TRADE || areaId == AREA_ALTAR) {
        return;
    }

    int seedCount = 2;
    int metalCount = 2;
    int relicCount = 2;

    switch (areaId) {
        case AREA_INDUSTRIAL:
            seedCount = 1;
            metalCount = 4;
            relicCount = 2;
            break;

        case AREA_FARMLAND:
            seedCount = 4;
            metalCount = 1;
            relicCount = 1;
            break;

        case AREA_RANDOM:
            seedCount = 1;
            metalCount = 2;
            relicCount = 3;
            break;

        case AREA_BASE:
            seedCount = 1;
            metalCount = 3;
            relicCount = 2;
            break;

        default:
            break;
    }

    placeRandomDropsOfType(mapData, INV_SEEDS, seedCount);
    placeRandomDropsOfType(mapData, INV_METAL, metalCount);
    placeRandomDropsOfType(mapData, INV_ALIEN_RELICS, relicCount);
}
