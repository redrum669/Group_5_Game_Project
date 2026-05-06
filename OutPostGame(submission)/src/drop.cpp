#include "drop.h"
#include "inventory.h"
#include "worldmap.h"

// Global array to manage all world item drops
Drop worldDrops[MAX_DROPS];

// Initialize all drop slots as inactive
void dropInit(uint8_t mapData[MAP_ROWS][MAP_COLS])
{
    for (int i = 0; i < MAX_DROPS; i++)
    {
        worldDrops[i].active = false;
    }

    // Basic item set for initial spawn
    int itemPool[6] = {
        INV_SEEDS, INV_METAL, INV_ALIEN_RELICS,
        INV_SEEDS, INV_METAL, INV_ALIEN_RELICS
    };
    int totalItems = 6;

    for (int i = 0; i < totalItems; i++)
    {
        int targetX, targetY;
        bool placedSuccess = false;

        // Try up to 200 times to place the item on valid ground
        for (int attempt = 0; attempt < 200; attempt++)
        {
            targetX = random(1, MAP_COLS - 1);
            targetY = random(1, MAP_ROWS - 1);

            // Skip if on collision tile
            if (mapData[targetY][targetX] == 1)
                continue;

            // Skip if already occupied
            if (dropExistsAt(targetX, targetY))
                continue;

            // Place the drop
            worldDrops[i] = { targetX, targetY, itemPool[i], true };
            placedSuccess = true;
            break;
        }

        // Avoid unused variable warning
        (void)placedSuccess;
    }
}

// Try to pick up a drop at given tile; returns item type or -1
int dropPickup(int tileX, int tileY, Inventory* inv)
{
    for (int i = 0; i < MAX_DROPS; i++)
    {
        if (worldDrops[i].active &&
            worldDrops[i].tileX == tileX &&
            worldDrops[i].tileY == tileY)
        {
            int item = worldDrops[i].itemType;
            worldDrops[i].active = false;

            // Add to inventory (skip medikit since it's instant use)
            if (item != INV_MEDKIT)
            {
                inventoryAdd(inv, item, 1);
            }
            return item;
        }
    }
    return -1;
}

// Check if any active drop exists at (tileX, tileY)
bool dropExistsAt(int tileX, int tileY)
{
    for (int i = 0; i < MAX_DROPS; i++)
    {
        if (worldDrops[i].active &&
            worldDrops[i].tileX == tileX &&
            worldDrops[i].tileY == tileY)
        {
            return true;
        }
    }
    return false;
}

// Add a new item drop to the first empty slot
void dropAdd(int tileX, int tileY, int itemType)
{
    for (int i = 0; i < MAX_DROPS; i++)
    {
        if (!worldDrops[i].active)
        {
            worldDrops[i] = { tileX, tileY, itemType, true };
            return;
        }
    }
}

// Get item type at position; return -1 if none
int dropTypeAt(int tileX, int tileY)
{
    for (int i = 0; i < MAX_DROPS; i++)
    {
        if (worldDrops[i].active &&
            worldDrops[i].tileX == tileX &&
            worldDrops[i].tileY == tileY)
        {
            return worldDrops[i].itemType;
        }
    }
    return -1;
}

// Spawn item randomly on valid ground; supports platformer rules
bool dropAddRandom(int itemType, uint8_t mapData[MAP_ROWS][MAP_COLS], int* outX, int* outY)
{
    bool usePlatformRules = worldmapIsPlatformerArea(worldmapGetSelectedArea());

    for (int attempt = 0; attempt < 200; attempt++)
    {
        int tx = random(1, MAP_COLS - 1);
        int ty = random(1, MAP_ROWS - 1);

        if (mapData[ty][tx] == 1)
            continue;
        if (dropExistsAt(tx, ty))
            continue;

        // Platformer: must place on top of solid ground
        if (usePlatformRules)
        {
            if (ty >= MAP_ROWS - 1)
                continue;
            if (mapData[ty + 1][tx] != 1)
                continue;
        }

        dropAdd(tx, ty, itemType);

        // Output position if requested
        if (outX != nullptr)
            *outX = tx;
        if (outY != nullptr)
            *outY = ty;

        return true;
    }

    return false;
}

// Place multiple random items of one type on the map
static void placeRandomDropsOfType(uint8_t mapData[MAP_ROWS][MAP_COLS],
                                   int itemType, int count)
{
    for (int i = 0; i < count; i++)
    {
        dropAddRandom(itemType, mapData, nullptr, nullptr);
    }
}

// Initialize drops based on current area type
void dropInitForArea(uint8_t mapData[MAP_ROWS][MAP_COLS], int areaId)
{
    // Clear all existing drops
    for (int i = 0; i < MAX_DROPS; i++)
    {
        worldDrops[i].active = false;
    }

    // Safe zones: no random loot
    if (areaId == AREA_HOME || areaId == AREA_TRADE || areaId == AREA_ALTAR)
    {
        return;
    }

    // Default loot counts
    int seedCount   = 2;
    int metalCount  = 2;
    int relicCount  = 2;

    // Adjust loot for each area type
    switch (areaId)
    {
        case AREA_INDUSTRIAL:
            seedCount  = 1;
            metalCount = 4;
            relicCount = 2;
            break;

        case AREA_FARMLAND:
            seedCount  = 4;
            metalCount = 1;
            relicCount = 1;
            break;

        case AREA_RANDOM:
            seedCount  = 1;
            metalCount = 2;
            relicCount = 3;
            break;

        case AREA_BASE:
            seedCount  = 1;
            metalCount = 3;
            relicCount = 2;
            break;

        default:
            break;
    }

    // Spawn the calculated loot
    placeRandomDropsOfType(mapData, INV_SEEDS, seedCount);
    placeRandomDropsOfType(mapData, INV_METAL, metalCount);
    placeRandomDropsOfType(mapData, INV_ALIEN_RELICS, relicCount);
}