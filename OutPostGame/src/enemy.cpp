#include "enemy.h"
#include "drop.h"
#include "worldmap.h"

// Global enemy array
Enemy enemies[MAX_ENEMIES];


// ------------------------------------------------------------------
// Internal helpers
// ------------------------------------------------------------------

// Apply armor mitigation for specific enemy attacks (laser, heavy).
// Armor is 0-100 and linearly reduces up to 70% of incoming damage.
// Armor durability decreases by roughly 1/3 of the prevented damage.
// Always inflicts at least 1 point.
static int applyArmorMitigation(Player* player, int rawDamage) {
    int reductionPct = player->armor;
    if (reductionPct > 70) reductionPct = 70;  // cap mitigation to 70%

    int prevented = (rawDamage * reductionPct) / 100;
    int dmg       = rawDamage - prevented;
    if (dmg < 1) dmg = 1;

    // Consume some armor durability proportional to prevented damage
    int armorLoss = (prevented + 2) / 3; // soften decay: about one third
    if (armorLoss > 0) {
        player->armor -= armorLoss;
        if (player->armor < 0) player->armor = 0;
    }
    return dmg;
}

static bool enemyCheckMapCollision(uint8_t mapData[MAP_ROWS][MAP_COLS], int px, int py, int w, int h) {
    int tx1 = px / TILE_SIZE;
    int ty1 = py / TILE_SIZE;
    int tx2 = (px + w - 1) / TILE_SIZE;
    int ty2 = (py + h - 1) / TILE_SIZE;

    if (tx1 < 0) tx1 = 0;
    if (tx2 >= MAP_COLS) tx2 = MAP_COLS - 1;

    for (int y = ty1; y <= ty2; y++) {
        for (int x = tx1; x <= tx2; x++) {
            if (y >= 0 && y < MAP_ROWS && x >= 0 && x < MAP_COLS) {
                if (mapData[y][x] == 1) return true;
            }
        }
    }
    return false;
}


// Tile is "occupied by an enemy" if any active enemy currently sits on it
// OR is mid-interpolation heading toward it (target-tile reservation).
bool enemyAt(int tileX, int tileY) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        if (enemies[i].tileX == tileX && enemies[i].tileY == tileY) return true;
        if (enemies[i].isMoving) {
            int tx = enemies[i].tileX + enemies[i].moveDirX;
            int ty = enemies[i].tileY + enemies[i].moveDirY;
            if (tx == tileX && ty == tileY) return true;
        }
    }
    return false;
}

// Player-tile reservation: player occupies its current tile, and during smooth
// movement also reserves the target tile so enemies never cross through.
static bool playerOccupies(Player* p, int tx, int ty) {
    if (p->tileX == tx && p->tileY == ty) return true;
    if (p->isMoving) {
        int ptx = p->tileX + p->moveDirX;
        int pty = p->tileY + p->moveDirY;
        if (ptx == tx && pty == ty) return true;
    }
    return false;
}

// Returns true if an enemy can step onto (tx,ty):
//   not out-of-bounds, not a wall, not already occupied by another enemy, not player
static bool enemyCanMove(uint8_t mapData[MAP_ROWS][MAP_COLS], int tx, int ty,
                         Player* player) {
    if (tx < 0 || ty < 0 || tx >= MAP_COLS || ty >= MAP_ROWS) return false;
    if (mapData[ty][tx] == 1) return false;        // wall
    if (enemyAt(tx, ty)) return false;              // another enemy
    if (player && playerOccupies(player, tx, ty)) return false;
    return true;
}

// ------------------------------------------------------------------
// enemyInit — spawn area-scaled enemies on random empty tiles
// ------------------------------------------------------------------
void enemyInit(uint8_t mapData[MAP_ROWS][MAP_COLS], int playerX, int playerY, int areaId) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].active = false;
        enemies[i].pendingDeathClear = false;
    }

    int types[MAX_ENEMIES] = {
        ENEMY_KNIFE,                 // Screen 1: 1 enemy
        ENEMY_LASER,                 // Screen 2: 1 enemy
        ENEMY_HEAVY, ENEMY_LASER,    // Screen 3: 2 enemies
        ENEMY_HEAVY, ENEMY_LASER     // Screen 4: 2 enemies
    };
    int enemyCount = 6;

    for (int i = 0; i < enemyCount; i++) {
        for (int attempt = 0; attempt < 300; attempt++) {
            int screenIdx = 0;
            if (i < 1) screenIdx = 0;
            else if (i < 2) screenIdx = 1;
            else if (i < 4) screenIdx = 2;
            else screenIdx = 3;

            int minX = screenIdx * VISIBLE_COLS;
            int maxX = minX + VISIBLE_COLS - 1;
            if (minX == 0) minX = 1;
            if (maxX >= MAP_COLS - 1) maxX = MAP_COLS - 2;

            int tx = random(minX, maxX + 1);
            int ty = random(1, MAP_ROWS - 1);

            if (mapData[ty][tx] == 1) continue;
            if (enemyAt(tx, ty))      continue;
            if (abs(tx - playerX) + abs(ty - playerY) < 5) continue;

            Enemy& e = enemies[i];
            e.tileX      = tx;   e.tileY      = ty;
            e.prevX      = tx;   e.prevY      = ty;
            e.hp         = (types[i] == ENEMY_HEAVY) ? HEAVY_HP :
                           (types[i] == ENEMY_LASER) ? LASER_HP :
                                                       KNIFE_HP;
            e.active     = true;
            e.moved      = false;
            e.lastMove   = 0;    e.lastAtk    = 0;
            e.type       = types[i];
            e.laserFired = false;
            e.alerted    = false;
            e.laserEndX  = tx;   e.laserEndY  = ty;
            // Smooth-movement init
            e.moveDirX      = 0;
            e.moveDirY      = 0;
            e.moveProgress  = 0;
            e.isMoving      = false;
            e.lastDrawnPixelX = PLAY_AREA_X + tx * TILE_SIZE;
            e.lastDrawnPixelY = PLAY_AREA_Y + ty * TILE_SIZE;

            // Platformer coords
            e.pixelX        = tx * TILE_SIZE;
            e.pixelY        = ty * TILE_SIZE;
            e.velY          = 0;
            e.grounded      = true;

            e.pendingDeathClear = false;
            break;
        }
    }
}

// ------------------------------------------------------------------
// enemyUpdate — called every frame; handles movement + attacks
// Returns true if any enemy moved (triggers map redraw in game.cpp)
// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Internal: pick one axis step toward player, start interpolation if free.
// Returns true if a new step was started.
// ------------------------------------------------------------------
static bool beginStepTowardPlayer(uint8_t mapData[MAP_ROWS][MAP_COLS],
                                   Enemy& e, Player* player) {
    int dx = player->tileX - e.tileX;
    int dy = player->tileY - e.tileY;
    int ndx = 0, ndy = 0;

    if (abs(dx) >= abs(dy)) {
        ndx = (dx > 0) ? 1 : (dx < 0 ? -1 : 0);
        if (ndx != 0 && !enemyCanMove(mapData, e.tileX + ndx, e.tileY, player)) {
            ndx = 0;
            if (dy != 0) ndy = (dy > 0) ? 1 : -1;
        }
    } else {
        ndy = (dy > 0) ? 1 : (dy < 0 ? -1 : 0);
        if (ndy != 0 && !enemyCanMove(mapData, e.tileX, e.tileY + ndy, player)) {
            ndy = 0;
            if (dx != 0) ndx = (dx > 0) ? 1 : -1;
        }
    }
    if (ndx == 0 && ndy == 0) return false;
    if (!enemyCanMove(mapData, e.tileX + ndx, e.tileY + ndy, player)) return false;

    e.moveDirX     = ndx;
    e.moveDirY     = ndy;
    e.moveProgress = 0;
    e.isMoving     = true;
    return true;
}

// Per-type pixels-per-frame speed while interpolating.
static uint8_t enemyStepSpeed(int type) {
    if (type == ENEMY_HEAVY) return HEAVY_MOVE_PX;
    if (type == ENEMY_LASER) return LASER_MOVE_PX;
    return KNIFE_MOVE_PX;
}

// ------------------------------------------------------------------
// enemyUpdate — runs every frame.
// Advances pixel-level interpolation; when a step finishes it commits the
// tile change and may start the next step (attack / shoot / walk).
// Returns true if any enemy's draw position changed this frame.
// ------------------------------------------------------------------
bool enemyUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player) {
    bool anyMoved = false;
    uint32_t now = millis();
    bool isPlatformer = worldmapIsPlatformerArea(worldmapGetSelectedArea());
    static uint8_t moveCounter = 0;
    moveCounter++;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        Enemy& e = enemies[i];

        if (isPlatformer) {
            anyMoved = true;
            // Gravity
            e.velY += GRAVITY_PX;
            if (e.velY > 8) e.velY = 8;
            e.pixelY += e.velY;
            e.grounded = false;

            // Vertical collision
            if (enemyCheckMapCollision(mapData, e.pixelX, e.pixelY, TILE_SIZE, TILE_SIZE)) {
                if (e.velY > 0) { // landing
                    e.pixelY = ((e.pixelY + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
                    e.grounded = true;
                } else if (e.velY < 0) { // hitting ceiling
                    e.pixelY = (e.pixelY / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
                }
                e.velY = 0;
            }

            // Fall out of map
            if (e.pixelY > MAP_ROWS * TILE_SIZE + TILE_SIZE) {
                e.hp = 0;
                e.active = false;
                continue;
            }

            int pxDistPlat = abs(player->pixelX - e.pixelX);
            int pyDistPlat = abs(player->pixelY - e.pixelY);
            if (!e.alerted && pxDistPlat < 64 && pyDistPlat < 64) {
                e.alerted = true;
            }

            if (e.alerted) {
                bool shouldMove = true;
                e.laserFired = false;

                if (e.type == ENEMY_LASER) {
                    int pyDistY = abs(player->pixelY - e.pixelY);
                    int pxDistX = abs(player->pixelX - e.pixelX);
                    // If player is roughly on the same level and within sight range
                    if (pyDistY <= 32 && pxDistX <= LASER_RANGE * TILE_SIZE) {
                        shouldMove = false; // Stand still to shoot
                        if (now - e.lastAtk >= (uint32_t)LASER_ATK_MS) {
                            int signX = (player->pixelX > e.pixelX) ? 1 : -1;
                            e.moveDirX = signX; // Face the player

                            int cx = (e.pixelX + TILE_SIZE / 2) / TILE_SIZE;
                            int cy = (e.pixelY + TILE_SIZE / 2) / TILE_SIZE;
                            int ptX = (player->pixelX + TILE_SIZE / 2) / TILE_SIZE;
                            bool hit = true;
                            int checkX = cx + signX;
                            while (checkX != ptX && checkX >= 0 && checkX < MAP_COLS) {
                                if (mapData[cy][checkX] == 1) { hit = false; break; }
                                checkX += signX;
                            }
                            if (hit) {
                                player->hp -= applyArmorMitigation(player, LASER_DMG);
                                if (player->hp < 0) player->hp = 0;
                            }
                            if (checkX < 0) checkX = 0;
                            if (checkX >= MAP_COLS) checkX = MAP_COLS - 1;
                            e.laserEndX  = checkX;
                            e.laserEndY  = cy;
                            e.laserFired = true;
                            e.lastAtk    = now;
                        }
                    }
                }

                if (shouldMove && (moveCounter % 3 != 0)) { // 33% slower by skipping 1 in 3 frames
                    // Chasing (horizontal)
                    int targetDir = (player->pixelX > e.pixelX) ? 1 : -1;
                    if (targetDir != 0) e.moveDirX = targetDir;

                    int dx = e.moveDirX * enemyStepSpeed(e.type);
                    e.pixelX += dx;

                    if (e.pixelX < 0) { e.pixelX = 0; }
                    int maxPx = MAP_COLS * TILE_SIZE - TILE_SIZE;
                    if (e.pixelX > maxPx) { e.pixelX = maxPx; }

                    if (enemyCheckMapCollision(mapData, e.pixelX, e.pixelY, TILE_SIZE, TILE_SIZE)) {
                        e.pixelX -= dx;
                        // Hitting a wall while chasing, just stop.
                    } else if (e.grounded) {
                        // Edge detection - don't fall off ledges
                        int edgeX = e.pixelX;
                        if (e.moveDirX > 0) edgeX += TILE_SIZE - 1;
                        if (!enemyCheckMapCollision(mapData, edgeX + e.moveDirX, e.pixelY + TILE_SIZE + 1, 1, 1)) {
                            e.pixelX -= dx;
                        }
                    }
                }
            }

            // Sync logical tile coords for attack range and drop logic
            e.tileX = (e.pixelX + TILE_SIZE / 2) / TILE_SIZE;
            e.tileY = (e.pixelY + TILE_SIZE / 2) / TILE_SIZE;

            // Attack player if close
            int pxDist = abs(player->pixelX - e.pixelX);
            int pyDist = abs(player->pixelY - e.pixelY);
            if (pxDist < TILE_SIZE && pyDist < TILE_SIZE) {
                uint32_t atkDelay = (e.type == ENEMY_HEAVY) ? HEAVY_ATK_MS : KNIFE_ATK_MS;
                int dmg = (e.type == ENEMY_HEAVY) ? HEAVY_DMG : KNIFE_DMG;
                if (e.type == ENEMY_LASER) { atkDelay = LASER_ATK_MS; dmg = LASER_DMG; }

                if (now - e.lastAtk >= atkDelay) {
                    player->hp -= applyArmorMitigation(player, dmg);
                    if (player->hp < 0) player->hp = 0;
                    e.lastAtk = now;
                }
            }
            continue;
        }

        // 1) Advance in-flight interpolation. If not finished this frame, done.
        if (e.isMoving) {
            anyMoved = true;
            uint8_t speed = enemyStepSpeed(e.type);
            e.moveProgress += speed;
            if (e.moveProgress < TILE_SIZE) continue;
            // Step complete — commit tile change, fall through to pick next action.
            e.prevX  = e.tileX;  e.prevY = e.tileY;
            e.tileX += e.moveDirX;
            e.tileY += e.moveDirY;
            e.moveDirX = 0; e.moveDirY = 0;
            e.moveProgress = 0;
            e.isMoving = false;
            e.moved = true;
            e.lastMove = now;
        }

        // 2) Recompute distance with committed tile.
        int dx   = player->tileX - e.tileX;
        int dy   = player->tileY - e.tileY;
        int dist = abs(dx) + abs(dy);

        if (!e.alerted && dist <= 4) {
            e.alerted = true;
        }

        if (!e.alerted) {
            continue;
        }

        // ── KNIFE: melee chase ─────────────────────────────────────
        if (e.type == ENEMY_KNIFE) {
            if (dist == 1) {
                if (now - e.lastAtk >= (uint32_t)KNIFE_ATK_MS) {
                    player->hp -= KNIFE_DMG;
                    if (player->hp < 0) player->hp = 0;
                    e.lastAtk = now;
                }
                continue;
            }
            if (beginStepTowardPlayer(mapData, e, player)) anyMoved = true;
        }

        // ── LASER: ranged straight-line + walks when out of range ──
        else if (e.type == ENEMY_LASER) {
            e.laserFired = false;
            if (dist <= LASER_RANGE) {
                if (now - e.lastAtk >= (uint32_t)LASER_ATK_MS) {
                    int sdx = 0, sdy = 0;
                    if (abs(dx) >= abs(dy)) sdx = (dx > 0) ? 1 : -1;
                    else                    sdy = (dy > 0) ? 1 : -1;

                    int rx = e.tileX + sdx, ry = e.tileY + sdy;
                    bool hit = false;
                    while (rx >= 0 && rx < MAP_COLS && ry >= 0 && ry < MAP_ROWS) {
                        if (mapData[ry][rx] == 1) break;
                        if (rx == player->tileX && ry == player->tileY) {
                            hit = true; break;
                        }
                        rx += sdx; ry += sdy;
                    }
                    if (hit) {
                        player->hp -= applyArmorMitigation(player, LASER_DMG);
                        if (player->hp < 0) player->hp = 0;
                    }
                    if (rx < 0)          rx = 0;
                    if (rx >= MAP_COLS)  rx = MAP_COLS - 1;
                    if (ry < 0)          ry = 0;
                    if (ry >= MAP_ROWS)  ry = MAP_ROWS - 1;
                    e.laserEndX  = rx; e.laserEndY  = ry;
                    e.laserFired = true;
                    e.lastAtk    = now;
                }
            }
            if (dist > LASER_RANGE) {
                if (beginStepTowardPlayer(mapData, e, player)) anyMoved = true;
            }
        }

        // ── HEAVY: slow hard-hitting melee ─────────────────────────
        else if (e.type == ENEMY_HEAVY) {
            if (dist == 1) {
                if (now - e.lastAtk >= (uint32_t)HEAVY_ATK_MS) {
                    player->hp -= applyArmorMitigation(player, HEAVY_DMG);
                    if (player->hp < 0) player->hp = 0;
                    e.lastAtk = now;
                }
                continue;
            }
            if (beginStepTowardPlayer(mapData, e, player)) anyMoved = true;
        }

        // Sync pixel coords for UI consistency
        e.pixelX = e.tileX * TILE_SIZE + e.moveDirX * e.moveProgress;
        e.pixelY = e.tileY * TILE_SIZE + e.moveDirY * e.moveProgress;
    }

    return anyMoved;
}

// ------------------------------------------------------------------
// playerAttackEnemy — called on BTN_A press
// Hits the first adjacent enemy; if killed, drops Alien Relic
// Returns true if an enemy was hit (triggers redraw)
// ------------------------------------------------------------------
bool playerAttackEnemy(int playerX, int playerY) {
    const int offsets[4][2] = {{0,-1},{0,1},{-1,0},{1,0}};

    for (int d = 0; d < 4; d++) {
        int tx = playerX + offsets[d][0];
        int ty = playerY + offsets[d][1];

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].active) continue;
            if (enemies[i].tileX != tx || enemies[i].tileY != ty) continue;

            enemies[i].hp -= PLAYER_ATK_DMG;
            if (enemies[i].hp <= 0) {
                // Drop an Alien Relic at the enemy's tile
                dropAdd(enemies[i].tileX, enemies[i].tileY, INV_ALIEN_RELICS);
                enemies[i].pendingDeathClear = true;
                enemies[i].active = false;
            }
            return true;  // hit something — stop after first target
        }
    }
    return false;
}

bool enemyAllDead() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) return false;
    }
    return true;
}

void enemyAlertAll() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            enemies[i].alerted = true;
        }
    }
}
