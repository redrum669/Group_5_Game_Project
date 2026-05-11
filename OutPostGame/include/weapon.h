#ifndef WEAPON_H
#define WEAPON_H

#include <Arduino.h>

#define WEAPON_SMG       0
#define WEAPON_MG        1
#define WEAPON_LASER     2
#define WEAPON_LASER_GUN 3
#define WEAPON_COUNT     4

#define SMG_DAMAGE        10
#define SMG_FIRE_RATE_MS  350
#define SMG_RANGE         4

#define MG_DAMAGE         15
#define MG_FIRE_RATE_MS   900
#define MG_RANGE          6

#define LASER_SWORD_DAMAGE       35
#define LASER_SWORD_FIRE_RATE_MS 300
#define LASER_SWORD_RANGE        2

#define LASER_GUN_DAMAGE         20
#define LASER_GUN_FIRE_RATE_MS   400
#define LASER_GUN_RANGE          8

#define WEAPON_UNLOCK_SMG       (1 << WEAPON_SMG)
#define WEAPON_UNLOCK_MG        (1 << WEAPON_MG)
#define WEAPON_UNLOCK_LASER     (1 << WEAPON_LASER)
#define WEAPON_UNLOCK_LASER_GUN (1 << WEAPON_LASER_GUN)

struct Weapon {
    int      type;
    uint32_t lastFire;
    int      lastTargetX;
    int      lastTargetY;
    uint8_t  unlockedMask;
    bool     isCharging;
    uint32_t chargeStartMs;
};

struct Projectile {
    bool     active;
    int      pixelX;
    int      pixelY;
    int      vx;
    int      vy;
    int      type;
    int      damage;
    int      range;
    uint16_t piercedMask;
    int      lastDrawnX;
    int      lastDrawnY;
    int      lastDrawnW;
    int      lastDrawnH;
    bool     pendingErase;
};

#define MAX_PROJECTILES 15
extern Projectile projectiles[MAX_PROJECTILES];

void projectilesInit();
void projectilesUpdate(uint8_t mapData[11][60]);

void weaponInit(Weapon* w);
void weaponSwitch(Weapon* w);
void weaponUnlock(Weapon* w, int weaponType);
bool weaponIsUnlocked(const Weapon* w, int weaponType);
int  weaponFire(Weapon* w, int pixelX, int pixelY, int dir, bool isPlatformer);

#endif
