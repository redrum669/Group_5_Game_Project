#include "enemy.h"
#include "drop.h"
#include "worldmap.h"

// Global array to manage all active enemies
Enemy enemies[MAX_ENEMIES];

// ------------------------------------------------------------------
// Internal Helper Functions
// ------------------------------------------------------------------

// Reduces damage using player armor (max 70% reduction)
// Lowers armor durability based on damage prevented
// Ensures at least 1 damage is dealt
static int applyArmorMitigation(Player* player, int rawDamage)
{
    int reductionPercent = player->armor;
    if (reductionPercent > 70)
        reductionPercent = 70;

    int preventedDamage = (rawDamage * reductionPercent) / 100;
    int finalDamage = rawDamage - preventedDamage;

    if (finalDamage < 1)
        finalDamage = 1;

    // Reduce armor durability
    int armorLoss = (preventedDamage + 2) / 3;
    if (armorLoss > 0)
    {
        player->armor -= armorLoss;
        if (player->armor < 0)
            player->armor = 0;
    }

    return finalDamage;
}

// Check collision between enemy and map tiles
static bool enemyCheckMapCollision(uint8_t mapData[MAP_ROWS][MAP_COLS], int px, int py, int w, int h)
{
    int tileX1 = px / TILE_SIZE;
    int tileY1 = py / TILE_SIZE;
    int tileX2 = (px + w - 1) / TILE_SIZE;
    int tileY2 = (py + h - 1) / TILE_SIZE;

    if (tileX1 < 0) tileX1 = 0;
    if (tileX2 >= MAP_COLS) tileX2 = MAP_COLS - 1;

    for (int y = tileY1; y <= tileY2; y++)
    {
        for (int x = tileX1; x <= tileX2; x++)
        {
            if (y >= 0 && y < MAP_ROWS && x >= 0 && x < MAP_COLS)
            {
                if (mapData[y][x] == 1)
                    return true;
            }
        }
    }
    return false;
}

// Check if any enemy is on or moving to a tile
bool enemyAt(int tileX, int tileY)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!enemies[i].active)
            continue;

        if (enemies[i].tileX == tileX && enemies[i].tileY == tileY)
            return true;

        if (enemies[i].isMoving)
        {
            int targetX = enemies[i].tileX + enemies[i].moveDirX;
            int targetY = enemies[i].tileY + enemies[i].moveDirY;
            if (targetX == tileX && targetY == tileY)
                return true;
        }
    }
    return false;
}

// Check if player is on or moving to a tile
static bool playerOccupies(Player* player, int tileX, int tileY)
{
    if (player->tileX == tileX && player->tileY == tileY)
        return true;

    if (player->isMoving)
    {
        int targetX = player->tileX + player->moveDirX;
        int targetY = player->tileY + player->moveDirY;
        if (targetX == tileX && targetY == tileY)
            return true;
    }
    return false;
}

// Check if enemy can move to a tile (no wall, no enemy, no player)
static bool enemyCanMove(uint8_t mapData[MAP_ROWS][MAP_COLS], int tileX, int tileY, Player* player)
{
    if (tileX < 0 || tileY < 0 || tileX >= MAP_COLS || tileY >= MAP_ROWS)
        return false;

    if (mapData[tileY][tileX] == 1)
        return false;

    if (enemyAt(tileX, tileY))
        return false;

    if (player && playerOccupies(player, tileX, tileY))
        return false;

    return true;
}

// Start moving one step toward the player if path is clear
static bool beginStepTowardPlayer(uint8_t mapData[MAP_ROWS][MAP_COLS], Enemy& enemy, Player* player)
{
    int dx = player->tileX - enemy.tileX;
    int dy = player->tileY - enemy.tileY;
    int newDirX = 0, newDirY = 0;

    if (abs(dx) >= abs(dy))
    {
        newDirX = (dx > 0) ? 1 : (dx < 0 ? -1 : 0);
        if (newDirX != 0 && !enemyCanMove(mapData, enemy.tileX + newDirX, enemy.tileY, player))
        {
            newDirX = 0;
            if (dy != 0)
                newDirY = (dy > 0) ? 1 : -1;
        }
    }
    else
    {
        newDirY = (dy > 0) ? 1 : (dy < 0 ? -1 : 0);
        if (newDirY != 0 && !enemyCanMove(mapData, enemy.tileX, enemy.tileY + newDirY, player))
        {
            newDirY = 0;
            if (dx != 0)
                newDirX = (dx > 0) ? 1 : -1;
        }
    }

    if (newDirX == 0 && newDirY == 0)
        return false;

    if (!enemyCanMove(mapData, enemy.tileX + newDirX, enemy.tileY + newDirY, player))
        return false;

    enemy.moveDirX = newDirX;
    enemy.moveDirY = newDirY;
    enemy.moveProgress = 0;
    enemy.isMoving = true;

    return true;
}

// Get movement speed (pixels per frame) for enemy type
static uint8_t enemyStepSpeed(int type)
{
    if (type == ENEMY_HEAVY)
        return HEAVY_MOVE_PX;
    if (type == ENEMY_LASER)
        return LASER_MOVE_PX;
    return KNIFE_MOVE_PX;
}

// ------------------------------------------------------------------
// Initialize enemies for current area
// ------------------------------------------------------------------
void enemyInit(uint8_t mapData[MAP_ROWS][MAP_COLS], int playerX, int playerY, int areaId)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        enemies[i].active = false;
        enemies[i].pendingDeathClear = false;
    }

    int enemyTypes[MAX_ENEMIES] = {
        ENEMY_KNIFE,
        ENEMY_LASER,
        ENEMY_HEAVY, ENEMY_LASER,
        ENEMY_HEAVY, ENEMY_LASER
    };
    int totalEnemies = 6;

    for (int i = 0; i < totalEnemies; i++)
    {
        for (int attempt = 0; attempt < 300; attempt++)
        {
            int screenIndex = 0;
            if (i < 1) screenIndex = 0;
            else if (i < 2) screenIndex = 1;
            else if (i < 4) screenIndex = 2;
            else screenIndex = 3;

            int minX = screenIndex * VISIBLE_COLS;
            int maxX = minX + VISIBLE_COLS - 1;
            if (minX == 0) minX = 1;
            if (maxX >= MAP_COLS - 1) maxX = MAP_COLS - 2;

            int tileX = random(minX, maxX + 1);
            int tileY = random(1, MAP_ROWS - 1);

            if (mapData[tileY][tileX] == 1)
                continue;
            if (enemyAt(tileX, tileY))
                continue;
            if (abs(tileX - playerX) + abs(tileY - playerY) < 5)
                continue;

            Enemy& e = enemies[i];
            e.tileX = tileX;
            e.tileY = tileY;
            e.prevX = tileX;
            e.prevY = tileY;

            if (enemyTypes[i] == ENEMY_HEAVY)
                e.hp = HEAVY_HP;
            else if (enemyTypes[i] == ENEMY_LASER)
                e.hp = LASER_HP;
            else
                e.hp = KNIFE_HP;

            e.active = true;
            e.moved = false;
            e.lastMove = 0;
            e.lastAtk = 0;
            e.type = enemyTypes[i];
            e.laserFired = false;
            e.alerted = false;
            e.laserEndX = tileX;
            e.laserEndY = tileY;

            e.moveDirX = 0;
            e.moveDirY = 0;
            e.moveProgress = 0;
            e.isMoving = false;

            e.lastDrawnPixelX = PLAY_AREA_X + tileX * TILE_SIZE;
            e.lastDrawnPixelY = PLAY_AREA_Y + tileY * TILE_SIZE;

            e.pixelX = tileX * TILE_SIZE;
            e.pixelY = tileY * TILE_SIZE;
            e.velY = 0;
            e.grounded = true;
            e.pendingDeathClear = false;

            break;
        }
    }
}

// ------------------------------------------------------------------
// Update all enemies (movement, AI, attacks)
// Returns true if any enemy moved (requires redraw)
// ------------------------------------------------------------------
bool enemyUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player)
{
    bool anyMoved = false;
    uint32_t currentTime = millis();
    bool isPlatformer = worldmapIsPlatformerArea(worldmapGetSelectedArea());
    static uint8_t moveFrameCounter = 0;
    moveFrameCounter++;

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!enemies[i].active)
            continue;

        Enemy& e = enemies[i];

        // --------------------------
        // Platformer physics (gravity, collision)
        // --------------------------
        if (isPlatformer)
        {
            anyMoved = true;

            // Apply gravity
            e.velY += GRAVITY_PX;
            if (e.velY > 8)
                e.velY = 8;
            e.pixelY += e.velY;
            e.grounded = false;

            // Vertical collision
            if (enemyCheckMapCollision(mapData, e.pixelX, e.pixelY, TILE_SIZE, TILE_SIZE))
            {
                if (e.velY > 0)
                {
                    e.pixelY = ((e.pixelY + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
                    e.grounded = true;
                }
                else if (e.velY < 0)
                {
                    e.pixelY = (e.pixelY / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
                }
                e.velY = 0;
            }

            // Kill if fallen off map
            if (e.pixelY > MAP_ROWS * TILE_SIZE + TILE_SIZE)
            {
                e.hp = 0;
                e.active = false;
                continue;
            }

            // Alert when player is near
            int pixelDistX = abs(player->pixelX - e.pixelX);
            int pixelDistY = abs(player->pixelY - e.pixelY);
            if (!e.alerted && pixelDistX < 64 && pixelDistY < 64)
            {
                e.alerted = true;
            }

            // Chase and attack
            if (e.alerted)
            {
                bool canMove = true;
                e.laserFired = false;

                // Laser enemy behavior
                if (e.type == ENEMY_LASER)
                {
                    int yDist = abs(player->pixelY - e.pixelY);
                    int xDist = abs(player->pixelX - e.pixelX);

                    if (yDist <= 32 && xDist <= LASER_RANGE * TILE_SIZE)
                    {
                        canMove = false;
                        if (currentTime - e.lastAtk >= (uint32_t)LASER_ATK_MS)
                        {
                            int dirX = (player->pixelX > e.pixelX) ? 1 : -1;
                            e.moveDirX = dirX;

                            int centerTileX = (e.pixelX + TILE_SIZE / 2) / TILE_SIZE;
                            int centerTileY = (e.pixelY + TILE_SIZE / 2) / TILE_SIZE;
                            int playerTileX = (player->pixelX + TILE_SIZE / 2) / TILE_SIZE;

                            bool lineClear = true;
                            int checkX = centerTileX + dirX;
                            while (checkX != playerTileX && checkX >= 0 && checkX < MAP_COLS)
                            {
                                if (mapData[centerTileY][checkX] == 1)
                                {
                                    lineClear = false;
                                    break;
                                }
                                checkX += dirX;
                            }

                            if (lineClear)
                            {
                                player->hp -= applyArmorMitigation(player, LASER_DMG);
                                if (player->hp < 0)
                                    player->hp = 0;
                            }

                            if (checkX < 0) checkX = 0;
                            if (checkX >= MAP_COLS) checkX = MAP_COLS - 1;

                            e.laserEndX = checkX;
                            e.laserEndY = centerTileY;
                            e.laserFired = true;
                            e.lastAtk = currentTime;
                        }
                    }
                }

                // Movement (slowed down)
                if (canMove && (moveFrameCounter % 3 != 0))
                {
                    int moveDir = (player->pixelX > e.pixelX) ? 1 : -1;
                    if (moveDir != 0)
                        e.moveDirX = moveDir;

                    int moveStep = e.moveDirX * enemyStepSpeed(e.type);
                    e.pixelX += moveStep;

                    if (e.pixelX < 0)
                        e.pixelX = 0;

                    int maxPixelX = MAP_COLS * TILE_SIZE - TILE_SIZE;
                    if (e.pixelX > maxPixelX)
                        e.pixelX = maxPixelX;

                    if (enemyCheckMapCollision(mapData, e.pixelX, e.pixelY, TILE_SIZE, TILE_SIZE))
                    {
                        e.pixelX -= moveStep;
                    }
                    else if (e.grounded)
                    {
                        int edgeCheckX = e.pixelX;
                        if (e.moveDirX > 0)
                            edgeCheckX += TILE_SIZE - 1;

                        if (!enemyCheckMapCollision(mapData, edgeCheckX + e.moveDirX, e.pixelY + TILE_SIZE + 1, 1, 1))
                        {
                            e.pixelX -= moveStep;
                        }
                    }
                }
            }

            // Sync tile position
            e.tileX = (e.pixelX + TILE_SIZE / 2) / TILE_SIZE;
            e.tileY = (e.pixelY + TILE_SIZE / 2) / TILE_SIZE;

            // Melee attack when close
            int distX = abs(player->pixelX - e.pixelX);
            int distY = abs(player->pixelY - e.pixelY);
            if (distX < TILE_SIZE && distY < TILE_SIZE)
            {
                uint32_t attackDelay;
                int damage;

                if (e.type == ENEMY_HEAVY)
                {
                    attackDelay = HEAVY_ATK_MS;
                    damage = HEAVY_DMG;
                }
                else if (e.type == ENEMY_LASER)
                {
                    attackDelay = LASER_ATK_MS;
                    damage = LASER_DMG;
                }
                else
                {
                    attackDelay = KNIFE_ATK_MS;
                    damage = KNIFE_DMG;
                }

                if (currentTime - e.lastAtk >= attackDelay)
                {
                    player->hp -= applyArmorMitigation(player, damage);
                    if (player->hp < 0)
                        player->hp = 0;
                    e.lastAtk = currentTime;
                }
            }

            continue;
        }

        // --------------------------
        // Turn-based tile movement
        // --------------------------
        if (e.isMoving)
        {
            anyMoved = true;
            uint8_t speed = enemyStepSpeed(e.type);
            e.moveProgress += speed;

            if (e.moveProgress < TILE_SIZE)
                continue;

            // Step finished
            e.prevX = e.tileX;
            e.prevY = e.tileY;
            e.tileX += e.moveDirX;
            e.tileY += e.moveDirY;

            e.moveDirX = 0;
            e.moveDirY = 0;
            e.moveProgress = 0;
            e.isMoving = false;
            e.moved = true;
            e.lastMove = currentTime;
        }

        // Distance to player
        int dx = player->tileX - e.tileX;
        int dy = player->tileY - e.tileY;
        int distance = abs(dx) + abs(dy);

        if (!e.alerted && distance <= 4)
            e.alerted = true;

        if (!e.alerted)
            continue;

        // Knife enemy: chase and melee
        if (e.type == ENEMY_KNIFE)
        {
            if (distance == 1)
            {
                if (currentTime - e.lastAtk >= (uint32_t)KNIFE_ATK_MS)
                {
                    player->hp -= KNIFE_DMG;
                    if (player->hp < 0)
                        player->hp = 0;
                    e.lastAtk = currentTime;
                }
                continue;
            }
            if (beginStepTowardPlayer(mapData, e, player))
                anyMoved = true;
        }

        // Laser enemy: ranged attack + chase when far
        else if (e.type == ENEMY_LASER)
        {
            e.laserFired = false;

            if (distance <= LASER_RANGE)
            {
                if (currentTime - e.lastAtk >= (uint32_t)LASER_ATK_MS)
                {
                    int shotDirX = 0, shotDirY = 0;
                    if (abs(dx) >= abs(dy))
                        shotDirX = (dx > 0) ? 1 : -1;
                    else
                        shotDirY = (dy > 0) ? 1 : -1;

                    int checkX = e.tileX + shotDirX;
                    int checkY = e.tileY + shotDirY;
                    bool hitPlayer = false;

                    while (checkX >= 0 && checkX < MAP_COLS && checkY >= 0 && checkY < MAP_ROWS)
                    {
                        if (mapData[checkY][checkX] == 1)
                            break;
                        if (checkX == player->tileX && checkY == player->tileY)
                        {
                            hitPlayer = true;
                            break;
                        }
                        checkX += shotDirX;
                        checkY += shotDirY;
                    }

                    if (hitPlayer)
                    {
                        player->hp -= applyArmorMitigation(player, LASER_DMG);
                        if (player->hp < 0)
                            player->hp = 0;
                    }

                    if (checkX < 0) checkX = 0;
                    if (checkX >= MAP_COLS) checkX = MAP_COLS - 1;
                    if (checkY < 0) checkY = 0;
                    if (checkY >= MAP_ROWS) checkY = MAP_ROWS - 1;

                    e.laserEndX = checkX;
                    e.laserEndY = checkY;
                    e.laserFired = true;
                    e.lastAtk = currentTime;
                }
            }

            if (distance > LASER_RANGE)
            {
                if (beginStepTowardPlayer(mapData, e, player))
                    anyMoved = true;
            }
        }

        // Heavy enemy: slow melee tank
        else if (e.type == ENEMY_HEAVY)
        {
            if (distance == 1)
            {
                if (currentTime - e.lastAtk >= (uint32_t)HEAVY_ATK_MS)
                {
                    player->hp -= applyArmorMitigation(player, HEAVY_DMG);
                    if (player->hp < 0)
                        player->hp = 0;
                    e.lastAtk = currentTime;
                }
                continue;
            }
            if (beginStepTowardPlayer(mapData, e, player))
                anyMoved = true;
        }

        // Update pixel position for rendering
        e.pixelX = e.tileX * TILE_SIZE + e.moveDirX * e.moveProgress;
        e.pixelY = e.tileY * TILE_SIZE + e.moveDirY * e.moveProgress;
    }

    return anyMoved;
}

// ------------------------------------------------------------------
// Player attacks adjacent enemy
// Returns true if hit
// ------------------------------------------------------------------
bool playerAttackEnemy(int playerX, int playerY)
{
    const int dirOffsets[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

    for (int d = 0; d < 4; d++)
    {
        int targetX = playerX + dirOffsets[d][0];
        int targetY = playerY + dirOffsets[d][1];

        for (int i = 0; i < MAX_ENEMIES; i++)
        {
            if (!enemies[i].active)
                continue;
            if (enemies[i].tileX != targetX || enemies[i].tileY != targetY)
                continue;

            enemies[i].hp -= PLAYER_ATK_DMG;

            if (enemies[i].hp <= 0)
            {
                dropAdd(enemies[i].tileX, enemies[i].tileY, INV_ALIEN_RELICS);
                enemies[i].pendingDeathClear = true;
                enemies[i].active = false;
            }

            return true;
        }
    }

    return false;
}

// Check if all enemies are dead
bool enemyAllDead()
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemies[i].active)
            return false;
    }
    return true;
}

// Alert all enemies to player position
void enemyAlertAll()
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemies[i].active)
        {
            enemies[i].alerted = true;
        }
    }
}