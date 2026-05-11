#include "portal.h"
#include "drop.h"
#include "enemy.h"

Portal portal = { false, -1, -1 };

void portalReset() {
    portal.active = false;
    portal.tileX  = -1;
    portal.tileY  = -1;
}

// Scan up to a 3×3 patch around each of the four corners.
// Picks a random starting corner so the portal feels unpredictable.
bool portalSpawn(uint8_t mapData[MAP_ROWS][MAP_COLS]) {
    // Look for fixed portal tile (9) first
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

    // Corner anchor + search direction: {anchorX, anchorY, stepX, stepY}
    int corners[4][4] = {
        {0,            0,            1,  1},   // top-left
        {MAP_COLS - 1, 0,           -1,  1},   // top-right
        {0,            MAP_ROWS - 1, 1, -1},   // bottom-left
        {MAP_COLS - 1, MAP_ROWS - 1,-1, -1},   // bottom-right
    };

    int start = random(4);
    for (int i = 0; i < 4; i++) {
        int ci = (start + i) % 4;
        int ax = corners[ci][0], ay = corners[ci][1];
        int dx = corners[ci][2], dy = corners[ci][3];

        for (int oy = 0; oy <= 2; oy++) {
            for (int ox = 0; ox <= 2; ox++) {
                int tx = ax + ox * dx;
                int ty = ay + oy * dy;
                if (tx < 0 || tx >= MAP_COLS || ty < 0 || ty >= MAP_ROWS) continue;
                if (mapData[ty][tx] != 0)    continue;  // wall
                if (dropExistsAt(tx, ty))    continue;  // drop already there
                if (enemyAt(tx, ty))         continue;  // enemy on tile
                portal.tileX  = tx;
                portal.tileY  = ty;
                portal.active = true;
                return true;
            }
        }
    }
    return false;   // map too crowded — very unlikely
}

bool portalNearPlayer(int playerX, int playerY) {
    if (!portal.active) return false;
    return (abs(playerX - portal.tileX) <= 1 &&
            abs(playerY - portal.tileY) <= 1);
}

