#include "portal.h"
#include "drop.h"
#include "enemy.h"

Portal portal = { false, -1, -1 };

void portalReset() {
    portal.active = false;
    portal.tileX = -1;
    portal.tileY = -1;
}

// Spawn portal at fixed tile (9) or at a valid corner position
bool portalSpawn(uint8_t mapData[MAP_ROWS][MAP_COLS]) {
    // First, check for fixed portal position marked as 9
    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLS; x++) {
            if (mapData[y][x] == 9) {
                portal.tileX = x;
                portal.tileY = y;
                portal.active = true;
                return true;
            }
        }
    }

    // Four corners with search directions
    int corners[4][4] = {
        { 0, 0, 1, 1 },
        { MAP_COLS - 1, 0, -1, 1 },
        { 0, MAP_ROWS - 1, 1, -1 },
        { MAP_COLS - 1, MAP_ROWS - 1, -1, -1 }
    };

    int startIndex = random(4);
    for (int i = 0; i < 4; i++) {
        int idx = (startIndex + i) % 4;
        int anchorX = corners[idx][0];
        int anchorY = corners[idx][1];
        int stepX = corners[idx][2];
        int stepY = corners[idx][3];

        for (int oy = 0; oy <= 2; oy++) {
            for (int ox = 0; ox <= 2; ox++) {
                int tx = anchorX + ox * stepX;
                int ty = anchorY + oy * stepY;

                if (tx < 0 || tx >= MAP_COLS || ty < 0 || ty >= MAP_ROWS)
                    continue;
                if (mapData[ty][tx] != 0)
                    continue;
                if (dropExistsAt(tx, ty))
                    continue;
                if (enemyAt(tx, ty))
                    continue;

                portal.tileX = tx;
                portal.tileY = ty;
                portal.active = true;
                return true;
            }
        }
    }
    return false;
}

// Check if player is adjacent to portal
bool portalNearPlayer(int playerX, int playerY) {
    if (!portal.active)
        return false;

    return (abs(playerX - portal.tileX) <= 1 &&
            abs(playerY - portal.tileY) <= 1);
}