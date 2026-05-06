#include "weapon.h"
#include "enemy.h"
#include "drop.h"
#include "character.h"

// Global array of all active/available projectiles
Projectile projectiles[MAX_PROJECTILES];

/**
 * @brief Initialize all projectiles to inactive state
 */
void projectilesInit(void) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectiles[i].active = false;
        projectiles[i].pendingErase = false;
        projectiles[i].piercedMask = 0;
    }
}

/**
 * @brief Update projectile positions, collisions, and lifetime
 * @param mapData Level collision map (1 = wall/solid)
 */
void projectilesUpdate(uint8_t mapData[11][60]) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        // Skip inactive projectiles
        if (!projectiles[i].active) {
            continue;
        }

        // Move projectile by velocity
        projectiles[i].pixelX += projectiles[i].vx;
        projectiles[i].pixelY += projectiles[i].vy;

        // Reduce range based on distance traveled
        projectiles[i].range -= abs(projectiles[i].vx) + abs(projectiles[i].vy);

        // Convert pixel position to tile coordinates (16x16 tiles)
        int px = projectiles[i].pixelX;
        int py = projectiles[i].pixelY;
        int tileX = px / 16;
        int tileY = py / 16;

        bool projectileHit = false;

        // Deactivate if maximum range reached
        if (projectiles[i].range <= 0) {
            projectileHit = true;
        }
        // Deactivate if out of map bounds or hitting a wall tile
        else if (tileX < 0 || tileX >= 60 || tileY < 0 || tileY >= 11 || mapData[tileY][tileX] == 1) {
            projectileHit = true;
        }
        // Check enemy collisions
        else {
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!enemies[e].active) {
                    continue;
                }

                // Get enemy position
                int enemyX = enemies[e].pixelX;
                int enemyY = enemies[e].pixelY;

                // Set hitbox Y range (extended for laser type)
                int hitTop = enemyY;
                int hitBot = enemyY + 16;
                if (projectiles[i].type == WEAPON_LASER) {
                    hitTop -= 16;
                    hitBot += 16;
                }

                // Check if projectile overlaps enemy hitbox
                if (px >= enemyX && px < enemyX + 16 && py >= hitTop && py < hitBot) {
                    // Handle piercing for laser gun (ignore already hit enemies)
                    if (projectiles[i].type == WEAPON_LASER_GUN) {
                        if (projectiles[i].piercedMask & (1 << e)) {
                            continue;
                        }
                        projectiles[i].piercedMask |= (1 << e);
                    }
                    // Non-piercing weapons despawn on hit
                    else {
                        projectileHit = true;
                    }

                    // Apply damage to enemy
                    enemies[e].hp -= projectiles[i].damage;

                    // Handle enemy death
                    if (enemies[e].hp <= 0) {
                        dropAdd(enemies[e].tileX, enemies[e].tileY, INV_ALIEN_RELICS);
                        enemies[e].pendingDeathClear = true;
                        enemies[e].active = false;
                    }

                    if (projectileHit) {
                        break;
                    }
                }
            }
        }

        // Mark projectile for removal if it hit something
        if (projectileHit) {
            projectiles[i].active = false;
            projectiles[i].pendingErase = true;
        }
    }
}

/**
 * @brief Initialize weapon default state
 * @param w Pointer to weapon struct
 */
void weaponInit(Weapon* w) {
    w->type          = WEAPON_SMG;
    w->lastFire      = 0;
    w->lastTargetX   = -1;
    w->lastTargetY   = -1;
    w->unlockedMask  = WEAPON_UNLOCK_SMG | WEAPON_UNLOCK_MG | WEAPON_UNLOCK_LASER;
    w->isCharging    = false;
    w->chargeStartMs = 0;
}

/**
 * @brief Switch to next unlocked weapon
 * @param w Pointer to weapon struct
 */
void weaponSwitch(Weapon* w) {
    // Cycle through weapon types to find next unlocked one
    for (int step = 1; step <= WEAPON_COUNT; step++) {
        int nextType = (w->type + step) % WEAPON_COUNT;
        if (weaponIsUnlocked(w, nextType)) {
            w->type = nextType;
            return;
        }
    }
}

/**
 * @brief Unlock a specific weapon type
 * @param w Pointer to weapon struct
 * @param weaponType Weapon type to unlock
 */
void weaponUnlock(Weapon* w, int weaponType) {
    if (weaponType >= 0 && weaponType < WEAPON_COUNT) {
        w->unlockedMask |= (1 << weaponType);
    }
}

/**
 * @brief Check if a weapon type is unlocked
 * @param w Pointer to weapon struct
 * @param weaponType Weapon type to check
 * @return true if unlocked, false otherwise
 */
bool weaponIsUnlocked(const Weapon* w, int weaponType) {
    if (weaponType < 0 || weaponType >= WEAPON_COUNT) {
        return false;
    }
    return (w->unlockedMask & (1 << weaponType)) != 0;
}

/**
 * @brief Fire a projectile from the current weapon
 * @param w Pointer to weapon struct
 * @param pixelX Player X position in pixels
 * @param pixelY Player Y position in pixels
 * @param dir Firing direction (CHAR_DIR_*)
 * @param isPlatformer True if using platformer movement controls
 * @return 1 if fired successfully, 0 otherwise
 */
int weaponFire(Weapon* w, int pixelX, int pixelY, int dir, bool isPlatformer) {
    uint32_t currentTime = millis();
    int fireSpeed = SMG_FIRE_RATE_MS;
    int dmg = SMG_DAMAGE;
    int maxRange = SMG_RANGE;
    int projectileSpeed = 12;

    // Set stats based on current weapon type
    switch (w->type) {
        case WEAPON_MG:
            fireSpeed = MG_FIRE_RATE_MS;
            dmg = MG_DAMAGE;
            maxRange = MG_RANGE;
            projectileSpeed = 6;
            break;
        case WEAPON_LASER:
            fireSpeed = LASER_SWORD_FIRE_RATE_MS;
            dmg = LASER_SWORD_DAMAGE;
            maxRange = LASER_SWORD_RANGE;
            projectileSpeed = 6;
            break;
        case WEAPON_LASER_GUN:
            fireSpeed = LASER_GUN_FIRE_RATE_MS;
            dmg = LASER_GUN_DAMAGE;
            maxRange = LASER_GUN_RANGE;
            projectileSpeed = 12;
            break;
        default:
            // SMG defaults already set
            break;
    }

    // Enforce cooldown ONLY for laser sword (sync with animation)
    if (w->type == WEAPON_LASER && (currentTime - w->lastFire) < (uint32_t)fireSpeed) {
        return 0;
    }
    w->lastFire = currentTime;

    // Alert all enemies when firing weapon
    enemyAlertAll();

    // Find an empty projectile slot
    int projectileIndex = -1;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active && !projectiles[i].pendingErase) {
            projectileIndex = i;
            break;
        }
    }
    if (projectileIndex < 0) {
        return 0; // No available projectiles
    }

    // Spawn new projectile
    Projectile* p = &projectiles[projectileIndex];
    p->active = true;
    p->pendingErase = false;
    p->type = w->type;
    p->damage = dmg;
    p->range = maxRange * 16; // Convert tiles to pixels
    p->piercedMask = 0;

    // Set projectile spawn position (offset from player)
    int spawnX = pixelX + 8;
    int spawnY = pixelY + 11;
    int velX = 0;
    int velY = 0;

    // Set direction and position offset
    if (isPlatformer) {
        // Platformer: left/right only
        if (dir == CHAR_DIR_LEFT) {
            velX = -projectileSpeed;
            spawnX -= 12;
        } else {
            velX = projectileSpeed;
            spawnX += 12;
        }
    } else {
        // Top-down: 4-directional movement
        if (dir == CHAR_DIR_UP) {
            velY = -projectileSpeed;
            spawnY -= 12;
        } else if (dir == CHAR_DIR_DOWN) {
            velY = projectileSpeed;
            spawnY += 12;
        } else if (dir == CHAR_DIR_LEFT) {
            velX = -projectileSpeed;
            spawnX -= 12;
        } else {
            velX = projectileSpeed;
            spawnX += 12;
        }
    }

    // Final projectile position and velocity
    p->pixelX = spawnX;
    p->pixelY = spawnY;
    p->vx = velX;
    p->vy = velY;
    p->lastDrawnX = spawnX;
    p->lastDrawnY = spawnY;

    return 1;
}