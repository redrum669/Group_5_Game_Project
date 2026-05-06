#ifndef WEAPON_CONTROL_H
#define WEAPON_CONTROL_H

#include <Arduino.h>

#define WEAPON_SMG         0
#define WEAPON_RIFLE       1
#define WEAPON_SWORD       2
#define WEAPON_LASER       3
#define MAX_PROJECTILES    12

struct Weapon {
  int type;
  uint32_t lastFireTime;
  bool charging;
};

struct Projectile {
  bool active;
  int x;
  int y;
  int velX;
  int velY;
  int damage;
};

extern struct Projectile bulletList[MAX_PROJECTILES];

void initWeapon(struct Weapon* w, int defaultType);
bool fireWeapon(struct Weapon* w, int x, int y, int dirX, int dirY);
void updateAllProjectiles(uint8_t map[MAP_GRID_ROWS][MAP_GRID_COLS]);
void drawProjectiles(void);

#endif