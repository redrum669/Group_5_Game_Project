#include "game1/entities/weapon.h"
#include "game1/entities/enemy.h"
#include "game1/entities/drop.h"
#include "game1/entities/character.h"

Projectile projectiles[MAX_PROJECTILES];

void projectilesInit() {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectiles[i].active = false;
        projectiles[i].pendingErase = false;
        projectiles[i].piercedMask = 0;
    }
}

void projectilesUpdate(uint8_t mapData[11][60]) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) continue;

        projectiles[i].pixelX += projectiles[i].vx;
        projectiles[i].pixelY += projectiles[i].vy;
        projectiles[i].range -= abs(projectiles[i].vx) + abs(projectiles[i].vy);

        int px = projectiles[i].pixelX;
        int py = projectiles[i].pixelY;
        int tx = px / 16; // TILE_SIZE
        int ty = py / 16;

        bool hit = false;

        // Despawn if range exhausted
        if (projectiles[i].range <= 0) {
            hit = true;
        }
        // Map bounds or wall
        else if (tx < 0 || tx >= 60 || ty < 0 || ty >= 11 || (mapData[ty][tx] == 1)) {
            hit = true;
        } else {
            // Check enemies
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!enemies[e].active) continue;
                int ex = enemies[e].pixelX;
                int ey = enemies[e].pixelY;
                int hitYTop = ey;
                int hitYBot = ey + 16;
                if (projectiles[i].type == WEAPON_LASER) {
                    hitYTop -= 16;
                    hitYBot += 16;
                }
                if (px >= ex && px < ex + 16 && py >= hitYTop && py < hitYBot) {
                    if (projectiles[i].type == WEAPON_LASER_GUN) {
                        if (projectiles[i].piercedMask & (1 << e)) continue;
                        projectiles[i].piercedMask |= (1 << e);
                    } else {
                        hit = true;
                    }
                    enemies[e].hp -= projectiles[i].damage;
                    if (enemies[e].hp <= 0) {
                        dropAdd(enemies[e].tileX, enemies[e].tileY, INV_ALIEN_RELICS);
                        enemies[e].pendingDeathClear = true;
                        enemies[e].active = false;
                    }
                    if (hit) break;
                }
            }
        }

        if (hit) {
            projectiles[i].active = false;
            projectiles[i].pendingErase = true;
        }
    }
}

void weaponInit(Weapon* w) {
    w->type          = WEAPON_SMG;
    w->lastFire      = 0;
    w->lastTargetX   = -1;
    w->lastTargetY   = -1;
    w->unlockedMask  = WEAPON_UNLOCK_SMG | WEAPON_UNLOCK_MG | WEAPON_UNLOCK_LASER;
    w->isCharging    = false;
    w->chargeStartMs = 0;
}

void weaponSwitch(Weapon* w) {
    for (int step = 1; step <= WEAPON_COUNT; step++) {
        int nextType = (w->type + step) % WEAPON_COUNT;
        if (weaponIsUnlocked(w, nextType)) {
            w->type = nextType;
            return;
        }
    }
}

void weaponUnlock(Weapon* w, int weaponType) {
    if (weaponType >= 0 && weaponType < WEAPON_COUNT) {
        w->unlockedMask |= (1 << weaponType);
    }
}

bool weaponIsUnlocked(const Weapon* w, int weaponType) {
    if (weaponType < 0 || weaponType >= WEAPON_COUNT) return false;
    return (w->unlockedMask & (1 << weaponType)) != 0;
}

int weaponFire(Weapon* w, int pixelX, int pixelY, int dir, bool isPlatformer) {
    uint32_t now = millis();
    int cooldown = SMG_FIRE_RATE_MS;
    int damage = SMG_DAMAGE;
    int range = SMG_RANGE;
    int speed = 12;

    if (w->type == WEAPON_MG) {
        cooldown = MG_FIRE_RATE_MS;
        damage = MG_DAMAGE;
        range = MG_RANGE;
        speed = 6;
    } else if (w->type == WEAPON_LASER) {
        cooldown = LASER_SWORD_FIRE_RATE_MS;
        damage = LASER_SWORD_DAMAGE;
        range = LASER_SWORD_RANGE;
        speed = 6; // thrust speed for swing animation
    } else if (w->type == WEAPON_LASER_GUN) {
        cooldown = LASER_GUN_FIRE_RATE_MS;
        damage = LASER_GUN_DAMAGE;
        range = LASER_GUN_RANGE;
        speed = 12;
    }

    // Cooldown enforced for LASER SWORD to sync with animation, removed for others
    if (w->type == WEAPON_LASER && now - w->lastFire < (uint32_t)cooldown) return 0;
    // if (now - w->lastFire < (uint32_t)cooldown) return 0; // old global cooldown
    w->lastFire = now;

    // Alert enemies when firing
    enemyAlertAll();

    int pIdx = -1;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active && !projectiles[i].pendingErase) {
            pIdx = i;
            break;
        }
    }
    if (pIdx < 0) return 0;

    projectiles[pIdx].active = true;
    projectiles[pIdx].pendingErase = false;
    projectiles[pIdx].type = w->type;
    projectiles[pIdx].damage = damage;
    projectiles[pIdx].range = range * 16;
    projectiles[pIdx].piercedMask = 0;

    int startX = pixelX + 8;
    int startY = pixelY + 11;

    int vx = 0;
    int vy = 0;
    if (isPlatformer) {
        if (dir == CHAR_DIR_LEFT) { vx = -speed; startX -= 12; }
        else { vx = speed; startX += 12; } // default right
    } else {
        if (dir == CHAR_DIR_UP) { vy = -speed; startY -= 12; }
        else if (dir == CHAR_DIR_DOWN) { vy = speed; startY += 12; }
        else if (dir == CHAR_DIR_LEFT) { vx = -speed; startX -= 12; }
        else if (dir == CHAR_DIR_RIGHT) { vx = speed; startX += 12; }
        else { vx = speed; startX += 12; } // default right
    }

    projectiles[pIdx].pixelX = startX;
    projectiles[pIdx].pixelY = startY;
    projectiles[pIdx].vx = vx;
    projectiles[pIdx].vy = vy;
    projectiles[pIdx].lastDrawnX = startX;
    projectiles[pIdx].lastDrawnY = startY;

    return 1;
}
