#include "game.h"
#include "config.h"
#include "st7789.h"
#include "input.h"
#include "ui.h"
#include "inventory.h"
#include "drop.h"
#include "enemy.h"
#include "weapon.h"
#include "portal.h"
#include "worldmap.h"
#include "cg.h"
#include "cg2.h"
#include "character.h"
#include "altar.h"
#include "supply.h"
#include "sound.h"

static GameState currentState = STATE_BOOT;
static GameState lastState    = STATE_BOOT;
static Player    player;

static uint32_t  bootTimer        = 0;
static bool      redrawRequired   = true;
static int       lastPlayerTileX  = -1;
static int       lastPlayerTileY  = -1;
static bool      inventoryVisible = false;
static bool      enemiesMoved     = false;
static bool      enemyKilledFlag  = false;
static bool      statusBarDirty   = false;
static bool      clearMedkitSpawned = false;
static int       clearMedkitTileX   = -1;
static int       clearMedkitTileY   = -1;
static bool      portalJustAppeared = false;
static bool      altarUnlockPlaying = false;
static uint32_t  altarUnlockTimer   = 0;
static bool      joySwitchReleased  = true;
static bool      tradeMenuOpen      = false;
static bool      tradeMenuDirty     = false;
static int       tradeMenuIndex     = 0;
static const char* tradeMessage     = "";
static bool      tradeMessageIsError = false;

static uint8_t testMap[MAP_ROWS][MAP_COLS] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// Check if a tile is blocked (wall, enemy, out of bounds, or solid entity)
static bool isTileBlocked(int tileX, int tileY) {
    if (tileX < 0 || tileY < 0 || tileX >= MAP_COLS || tileY >= MAP_ROWS)
        return true;

    if (!worldmapIsPlatformerArea(worldmapGetSelectedArea()) && tileX >= 14)
        return true;

    if (testMap[tileY][tileX] == 1)
        return true;

    if (worldmapGetSelectedArea() == AREA_TRADE && supplyIsSolidEntity(tileX, tileY))
        return true;

    if (enemyAt(tileX, tileY))
        return true;

    return false;
}

// Check collision between a rectangle and map walls
static bool checkMapCollision(int pixelX, int pixelY, int width, int height) {
    int tileX1 = pixelX / TILE_SIZE;
    int tileY1 = pixelY / TILE_SIZE;
    int tileX2 = (pixelX + width - 1) / TILE_SIZE;
    int tileY2 = (pixelY + height - 1) / TILE_SIZE;

    if (tileX1 < 0) tileX1 = 0;
    if (tileX2 >= MAP_COLS) tileX2 = MAP_COLS - 1;

    for (int y = tileY1; y <= tileY2; y++) {
        for (int x = tileX1; x <= tileX2; x++) {
            if (y >= 0 && y < MAP_ROWS && x >= 0 && x < MAP_COLS) {
                if (testMap[y][x] == 1)
                    return true;
            }
        }
    }
    return false;
}

// Snap all entity draw positions to current tile (for clean redraws)
static void latchEntityDrawPositions(Player* p) {
    p->moveDirX = 0;
    p->moveDirY = 0;
    p->moveProgress = 0;
    p->isMoving = false;

    bool isPlatformer = worldmapIsPlatformerArea(worldmapGetSelectedArea());

    if (isPlatformer) {
        p->lastDrawnPixelX = PLAY_AREA_X + p->pixelX - p->cameraX;
        p->lastDrawnPixelY = PLAY_AREA_Y + p->pixelY;
    } else {
        p->lastDrawnPixelX = PLAY_AREA_X + p->tileX * TILE_SIZE;
        p->lastDrawnPixelY = PLAY_AREA_Y + p->tileY * TILE_SIZE;
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;

        enemies[i].moveDirX = 0;
        enemies[i].moveDirY = 0;
        enemies[i].moveProgress = 0;
        enemies[i].isMoving = false;

        if (isPlatformer) {
            enemies[i].lastDrawnPixelX = PLAY_AREA_X + enemies[i].pixelX - p->cameraX;
            enemies[i].lastDrawnPixelY = PLAY_AREA_Y + enemies[i].pixelY;
        } else {
            enemies[i].lastDrawnPixelX = PLAY_AREA_X + enemies[i].tileX * TILE_SIZE;
            enemies[i].lastDrawnPixelY = PLAY_AREA_Y + enemies[i].tileY * TILE_SIZE;
        }
    }
}

// Check if current area should spawn a medkit when all enemies are dead
static bool shouldSpawnClearMedkit() {
    int area = worldmapGetSelectedArea();
    return area != AREA_HOME && area != AREA_TRADE && area != AREA_ALTAR;
}

// Execute selected trade in supply zone
static bool performSupplyTrade() {
    switch (tradeMenuIndex) {
        case 0:
            if (player.inv.seeds < 3) {
                tradeMessage = "Need 3 Seeds";
                tradeMessageIsError = true;
                return false;
            }
            inventoryAdd(&player.inv, INV_SEEDS, -3);
            inventoryAdd(&player.inv, INV_FOOD, 1);
            tradeMessage = "Traded Seeds for Food";
            tradeMessageIsError = false;
            return true;

        case 1:
            if (player.inv.metal < 4) {
                tradeMessage = "Need 4 Metal";
                tradeMessageIsError = true;
                return false;
            }
            inventoryAdd(&player.inv, INV_METAL, -4);
            inventoryAdd(&player.inv, INV_FOOD, 2);
            tradeMessage = "Traded Metal for Rations";
            tradeMessageIsError = false;
            return true;

        case 2:
            if (player.inv.alienRelics < 2) {
                tradeMessage = "Need 2 Relics";
                tradeMessageIsError = true;
                return false;
            }
            inventoryAdd(&player.inv, INV_ALIEN_RELICS, -2);
            inventoryAdd(&player.inv, INV_METAL, 3);
            tradeMessage = "Traded Relics for Metal";
            tradeMessageIsError = false;
            return true;

        default:
            if (player.inv.food < 6) {
                tradeMessage = "Need 6 Food";
                tradeMessageIsError = true;
                return false;
            }
            inventoryAdd(&player.inv, INV_FOOD, -6);
            player.hp = 100;
            statusBarDirty = true;
            tradeMessage = "Blood pack restored all HP";
            tradeMessageIsError = false;
            return true;
    }
}

// Boot screen logic
static void updateBootScreen() {
    if (millis() - bootTimer > 200) {
        cgInit();
        currentState = STATE_CG;
    }
}

// Opening cutscene logic
static void updateCutscene() {
    cgDrawUpdate();
    if (cgUpdate()) {
        currentState = STATE_MENU;
    }
}

// Ending cutscene logic
static void updateFinalCutscene() {
    cg2DrawUpdate();
    if (cg2Update()) {
        currentState = STATE_MENU;
    }
}

// Main menu input logic
static void updateMainMenu() {
    static bool lastAPress = false;
    bool currentAPress = isButtonAPressed();

    if (currentAPress && !lastAPress) {
        Serial.println("STOP_MUSIC");
        currentState = STATE_WORLDMAP;
        redrawRequired = true;
    }
    lastAPress = currentAPress;
}

// Main exploration gameplay update
static void updateExploration() {
    static uint32_t lastHungerTimer  = 0;
    static uint32_t lastStarveTimer  = 0;
    static uint32_t lastJoyToggle    = 0;
    static uint32_t lastTradeNav     = 0;
    static bool lastBtnA = false;
    static bool lastBtnB = false;
    static bool lastBtnC = false;
    static bool joyLatched = false;
    static bool nearPortal = false;
    static bool lastTradeUp = false;
    static bool lastTradeDown = false;

    bool joyPressed = isJoySWPressed();
    bool btnAPressed = isButtonAPressed();
    bool btnBPressed = isButtonBPressed();
    bool btnCPressed = isButtonCPressed();

    // Joy switch toggle for inventory / trade
    if (joySwitchReleased) {
        if (!joyPressed) {
            joySwitchReleased = false;
            joyLatched = false;
        }
    } else if (!joyPressed) {
        joyLatched = false;
    } else if (!joyLatched && millis() - lastJoyToggle > 180) {
        if (tradeMenuOpen) {
            tradeMenuOpen = false;
            tradeMenuDirty = false;
        } else {
            inventoryVisible = !inventoryVisible;
        }
        joyLatched = true;
        lastJoyToggle = millis();
    }

    // Button B: Eat / Trade / Unlock / Switch Weapon
    if (btnBPressed && !lastBtnB) {
        if (tradeMenuOpen) {
            performSupplyTrade();
            tradeMenuDirty = true;
        } else if (inventoryVisible) {
            if (player.inv.food > 0) {
                inventoryAdd(&player.inv, INV_FOOD, -1);
                player.hunger += FOOD_HUNGER_RESTORE;
                if (player.hunger > 100) player.hunger = 100;
                drawInventoryFoodCount(player.inv.food);
                drawStatusBar(&player);
            }
        } else {
            bool nearMerchant = (worldmapGetSelectedArea() == AREA_TRADE) && supplyNearMerchant(player.tileX, player.tileY);
            bool nearAltar = (worldmapGetSelectedArea() == AREA_ALTAR) && altarNearOffering(player.tileX, player.tileY);
            bool canOffer = nearAltar && (player.inv.alienRelics > 0);

            if (nearMerchant) {
                tradeMenuOpen = true;
                tradeMenuIndex = 0;
                tradeMessage = "";
                tradeMessageIsError = false;
                tradeMenuDirty = true;
            } else if (canOffer) {
                inventoryAdd(&player.inv, INV_ALIEN_RELICS, -player.inv.alienRelics);
                weaponUnlock(&player.weapon, WEAPON_LASER);
                weaponUnlock(&player.weapon, WEAPON_LASER_GUN);
                player.weapon.type = WEAPON_LASER_GUN;
                altarUnlockPlaying = true;
                altarUnlockTimer = millis();
                redrawRequired = true;
            } else if (nearAltar) {
                redrawRequired = true;
            } else {
                weaponSwitch(&player.weapon);
                playWeaponSwapSound();
                drawWeaponSlot(player.weapon.type);
            }
        }
    }
    lastBtnB = btnBPressed;

    // Close trade with A
    if (tradeMenuOpen && btnAPressed && !lastBtnA) {
        tradeMenuOpen = false;
        tradeMenuDirty = false;
        lastBtnA = btnAPressed;
        return;
    }

    // Trade menu navigation
    if (tradeMenuOpen) {
        bool upPressed = isUpPressed();
        bool downPressed = isDownPressed();

        if (millis() - lastTradeNav > 160) {
            if (upPressed && !lastTradeUp) {
                tradeMenuIndex--;
                if (tradeMenuIndex < 0) tradeMenuIndex = 3;
                tradeMessage = "";
                tradeMessageIsError = false;
                tradeMenuDirty = true;
                lastTradeNav = millis();
            } else if (downPressed && !lastTradeDown) {
                tradeMenuIndex++;
                if (tradeMenuIndex > 3) tradeMenuIndex = 0;
                tradeMessage = "";
                tradeMessageIsError = false;
                tradeMenuDirty = true;
                lastTradeNav = millis();
            }
        }
        lastTradeUp = upPressed;
        lastTradeDown = downPressed;
        lastBtnA = btnAPressed;
        return;
    }

    // Pause everything if inventory is open
    if (inventoryVisible) {
        if (btnCPressed && !lastBtnC) {
            if (player.inv.armor > 0) {
                inventoryAdd(&player.inv, INV_ARMOR, -1);
                player.armor += ARMOR_RESTORE;
                if (player.armor > 100) player.armor = 100;
                drawInventoryArmorCount(player.inv.armor);
                drawStatusBar(&player);
            }
        }
        lastBtnC = btnCPressed;
        return;
    }

    // Skip movement during altar animation
    if (altarUnlockPlaying) {
        if (millis() - altarUnlockTimer >= 7200) {
            altarUnlockPlaying = false;
            redrawRequired = true;
            drawWeaponSlot(player.weapon.type);
        }
        return;
    }

    // Hunger system
    if (millis() - lastHungerTimer >= HUNGER_DRAIN_INTERVAL) {
        lastHungerTimer = millis();
        if (player.hunger > 0) {
            player.hunger--;
            statusBarDirty = true;
        }
    }

    // Starvation damage
    if (player.hunger == 0 && millis() - lastStarveTimer >= STARVATION_INTERVAL) {
        lastStarveTimer = millis();
        player.hp -= STARVATION_DAMAGE;
        if (player.hp < 0) player.hp = 0;
        statusBarDirty = true;
    }

    // Enemy AI
    int oldHP = player.hp;
    int oldArmor = player.armor;
    if (enemyUpdate(testMap, &player)) {
        enemiesMoved = true;
    }
    if (player.hp != oldHP || player.armor != oldArmor) {
        statusBarDirty = true;
    }

    // Draw laser beams
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        if (enemies[i].laserFired) {
            drawLaserFlash(testMap, enemies[i].tileX, enemies[i].tileY, enemies[i].laserEndX, enemies[i].laserEndY, &player);
            enemies[i].laserFired = false;
        }
    }

    // Button A: Enter portal / trigger ending
    if (btnAPressed && !lastBtnA) {
        if (portal.active && portalNearPlayer(player.tileX, player.tileY)) {
            currentState = STATE_WORLDMAP;
            redrawRequired = true;
        } else if (worldmapGetSelectedArea() == AREA_HOME) {
            cg2Init();
            currentState = STATE_CG2;
            redrawRequired = true;
        }
    }
    lastBtnA = btnAPressed;

    // Button C: Fire weapon
    bool isPlatformer = worldmapIsPlatformerArea(worldmapGetSelectedArea());
    if (player.weapon.type == WEAPON_LASER_GUN) {
        if (btnCPressed) {
            if (!player.weapon.isCharging) {
                player.weapon.isCharging = true;
                player.weapon.chargeStartMs = millis();
            }
            updateChargingSound(true, player.weapon.chargeStartMs);
        } else {
            if (player.weapon.isCharging) {
                uint32_t charge = millis() - player.weapon.chargeStartMs;
                player.weapon.isCharging = false;
                updateChargingSound(false, 0);

                if (charge >= 1000) {
                    player.weapon.lastFire = millis();
                    enemyAlertAll();
                    playWeaponFire(WEAPON_LASER_GUN);

                    int dir = characterGetDir();
                    if (isPlatformer && dir != CHAR_DIR_LEFT) dir = CHAR_DIR_RIGHT;

                    int pLeft = player.pixelX;
                    int pRight = player.pixelX + 15;
                    int pTop = player.pixelY;
                    int pBottom = player.pixelY + 15;

                    if (isPlatformer) {
                        if (dir == CHAR_DIR_LEFT) { pLeft = 0; pRight = player.pixelX; }
                        else { pLeft = player.pixelX; pRight = MAP_COLS * TILE_SIZE; }
                    } else {
                        if (dir == CHAR_DIR_LEFT)  { pLeft = 0; pRight = player.pixelX; }
                        if (dir == CHAR_DIR_RIGHT) { pLeft = player.pixelX; pRight = MAP_COLS * TILE_SIZE; }
                        if (dir == CHAR_DIR_UP)    { pTop = 0; pBottom = player.pixelY; }
                        if (dir == CHAR_DIR_DOWN)  { pTop = player.pixelY; pBottom = MAP_ROWS * TILE_SIZE; }
                    }

                    bool hitAny = false;
                    for (int e = 0; e < MAX_ENEMIES; e++) {
                        if (enemies[e].active) {
                            int ex = enemies[e].pixelX + 8;
                            int ey = enemies[e].pixelY + 8;
                            if (ex >= pLeft && ex <= pRight && ey >= pTop && ey <= pBottom) {
                                enemies[e].hp = 0;
                                dropAdd(enemies[e].tileX, enemies[e].tileY, INV_ALIEN_RELICS);
                                enemies[e].active = false;
                                hitAny = true;
                            }
                        }
                    }

                    int recoilX = 0, recoilY = 0;
                    if (dir == CHAR_DIR_LEFT)  recoilX = 16;
                    if (dir == CHAR_DIR_RIGHT) recoilX = -16;
                    if (!isPlatformer && dir == CHAR_DIR_UP)   recoilY = 16;
                    if (!isPlatformer && dir == CHAR_DIR_DOWN) recoilY = -16;

                    int oldPX = player.pixelX;
                    int oldPY = player.pixelY;
                    player.pixelX += recoilX;
                    player.pixelY += recoilY;

                    if (player.pixelX < 0) player.pixelX = 0;
                    if (player.pixelX > MAP_COLS * TILE_SIZE - TILE_SIZE) player.pixelX = MAP_COLS * TILE_SIZE - TILE_SIZE;
                    if (player.pixelY < 0) player.pixelY = 0;
                    if (player.pixelY > MAP_ROWS * TILE_SIZE - TILE_SIZE) player.pixelY = MAP_ROWS * TILE_SIZE - TILE_SIZE;

                    if (checkMapCollision(player.pixelX, player.pixelY, TILE_SIZE, TILE_SIZE)) {
                        if (recoilX > 0) player.pixelX = ((player.pixelX + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
                        if (recoilX < 0) player.pixelX = (player.pixelX / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
                        if (recoilY > 0) player.pixelY = ((player.pixelY + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
                        if (recoilY < 0) player.pixelY = (player.pixelY / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
                    }

                    int realRecoilX = player.pixelX - oldPX;
                    int realRecoilY = player.pixelY - oldPY;
                    player.pixelX = oldPX;
                    player.pixelY = oldPY;

                    drawLaserCannonFlash(testMap, player.tileX, player.tileY, dir, isPlatformer, &player, realRecoilX, realRecoilY);

                    player.pixelX = oldPX + realRecoilX;
                    player.pixelY = oldPY + realRecoilY;
                    player.tileX = (player.pixelX + 8) / TILE_SIZE;
                    player.tileY = (player.pixelY + 8) / TILE_SIZE;

                    redrawRequired = true;
                }
            }
        }
    } else {
        if (btnCPressed && !lastBtnC) {
            int fired = weaponFire(&player.weapon, player.pixelX, player.pixelY, characterGetDir(), isPlatformer);
            if (fired) {
                playWeaponFire(player.weapon.type);
            }
            if (fired && isPlatformer && player.weapon.type == WEAPON_LASER) {
                int dash = (characterGetDir() == CHAR_DIR_LEFT) ? -8 : 8;
                player.pixelX += dash;
                if (player.pixelX < 0) player.pixelX = 0;
                if (player.pixelX > MAP_COLS * TILE_SIZE - TILE_SIZE) player.pixelX = MAP_COLS * TILE_SIZE - TILE_SIZE;

                if (checkMapCollision(player.pixelX, player.pixelY, TILE_SIZE, TILE_SIZE)) {
                    if (dash > 0) player.pixelX = ((player.pixelX + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
                    else player.pixelX = (player.pixelX / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
                }
            }
        }
    }
    lastBtnC = btnCPressed;

    // Update projectiles
    projectilesUpdate(testMap);

    // Check for enemy deaths
    bool enemyJustDied = false;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active && enemies[i].pendingDeathClear) {
            enemyJustDied = true;
        }
    }

    if (enemyJustDied && !enemyKilledFlag) {
        enemyKilledFlag = true;
        statusBarDirty = true;

        if (enemyAllDead() && !portal.active) {
            if (shouldSpawnClearMedkit() && dropAddRandom(INV_MEDKIT, testMap, &clearMedkitTileX, &clearMedkitTileY)) {
                clearMedkitSpawned = true;
            }
            if (portalSpawn(testMap)) {
                portalJustAppeared = true;
            }
        }
    }

    // Player movement
    if (isPlatformer) {
        // Platformer movement
        int dir = -1;
        int dx = 0;

        if (isLeftPressed())  { dx = -PLAYER_MOVE_PX; dir = CHAR_DIR_LEFT; }
        if (isRightPressed()) { dx = PLAYER_MOVE_PX;  dir = CHAR_DIR_RIGHT; }

        if (dx != 0) {
            player.pixelX += dx;
            if (player.pixelX < 0) player.pixelX = 0;
            if (player.pixelX > MAP_COLS * TILE_SIZE - TILE_SIZE) player.pixelX = MAP_COLS * TILE_SIZE - TILE_SIZE;

            if (checkMapCollision(player.pixelX, player.pixelY, TILE_SIZE, TILE_SIZE)) {
                if (dx > 0) player.pixelX = ((player.pixelX + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
                else player.pixelX = (player.pixelX / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
            }
        }

        // Jumping
        if (player.grounded && isUpPressed()) {
            player.velY = -JUMP_FORCE_PX;
            player.grounded = false;
            playJumpSound();
        }

        // Gravity
        player.velY += GRAVITY_PX;
        if (player.velY > 8) player.velY = 8;
        player.pixelY += player.velY;
        player.grounded = false;

        // Ground collision
        if (checkMapCollision(player.pixelX, player.pixelY, TILE_SIZE, TILE_SIZE)) {
            if (player.velY > 0) {
                player.pixelY = ((player.pixelY + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
                player.grounded = true;
            } else if (player.velY < 0) {
                player.pixelY = (player.pixelY / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
            }
            player.velY = 0;
        }

        // Fall damage / death
        if (player.pixelY > MAP_ROWS * TILE_SIZE + TILE_SIZE) {
            player.hp = 0;
            statusBarDirty = true;
        }

        if (dir != -1) characterSetDir(dir);

        // Update tile position
        int oldTX = player.tileX;
        int oldTY = player.tileY;
        player.tileX = (player.pixelX + TILE_SIZE / 2) / TILE_SIZE;
        player.tileY = (player.pixelY + TILE_SIZE / 2) / TILE_SIZE;

        // Pickup items
        if (player.tileX != oldTX || player.tileY != oldTY) {
            int picked = dropPickup(player.tileX, player.tileY, &player.inv);
            if (picked == INV_MEDKIT) {
                player.hp += MEDKIT_HEAL;
                if (player.hp > 100) player.hp = 100;
                statusBarDirty = true;
            }
        }

        // Camera
        int roomW = PLAY_AREA_W;
        int newCamX = (player.pixelX / roomW) * roomW;
        int maxCamX = (MAP_COLS * TILE_SIZE / roomW) * roomW - roomW;
        if (maxCamX < 0) maxCamX = 0;
        if (newCamX > maxCamX) newCamX = maxCamX;

        if (newCamX != player.cameraX) {
            player.cameraX = newCamX;
            redrawRequired = true;
            projectilesInit();
        }

    } else {
        // Top-down movement
        if (player.isMoving) {
            player.moveProgress += PLAYER_MOVE_PX;
            if (player.moveProgress >= TILE_SIZE) {
                player.tileX += player.moveDirX;
                player.tileY += player.moveDirY;
                player.moveDirX = 0;
                player.moveDirY = 0;
                player.moveProgress = 0;
                player.isMoving = false;

                int picked = dropPickup(player.tileX, player.tileY, &player.inv);
                if (picked == INV_MEDKIT) {
                    player.hp += MEDKIT_HEAL;
                    if (player.hp > 100) player.hp = 100;
                    statusBarDirty = true;
                }
            }
        }

        if (!player.isMoving) {
            int nx = 0, ny = 0, dir = -1;
            if (isUpPressed())    { ny = -1; dir = CHAR_DIR_UP;    }
            if (isDownPressed())  { ny =  1; dir = CHAR_DIR_DOWN;  }
            if (isLeftPressed())  { nx = -1; dir = CHAR_DIR_LEFT;  }
            if (isRightPressed()) { nx =  1; dir = CHAR_DIR_RIGHT; }

            if (nx != 0 || ny != 0) {
                int tgtX = player.tileX + nx;
                int tgtY = player.tileY + ny;
                if (!isTileBlocked(tgtX, tgtY)) {
                    player.moveDirX = nx;
                    player.moveDirY = ny;
                    player.moveProgress = 0;
                    player.isMoving = true;
                    characterSetDir(dir);
                }
            }
        }

        player.pixelX = player.tileX * TILE_SIZE + player.moveDirX * player.moveProgress;
        player.pixelY = player.tileY * TILE_SIZE + player.moveDirY * player.moveProgress;
        player.cameraX = 0;
    }

    uiSetCameraX(player.cameraX);

    // Update portal UI
    bool nearPortalNow = portalNearPlayer(player.tileX, player.tileY);
    if (nearPortalNow != nearPortal) {
        drawPortalSlot(nearPortalNow);
        nearPortal = nearPortalNow;
    }
}

// Initialize game state
void gameInit() {
    Serial.println("STOP_MUSIC");
    bootTimer = millis();

    player.tileX = 2;
    player.tileY = 2;
    player.hp = 100;
    player.hunger = 100;
    player.armor = 100;

    player.moveDirX = 0;
    player.moveDirY = 0;
    player.moveProgress = 0;
    player.isMoving = false;
    player.lastDrawnPixelX = PLAY_AREA_X + player.tileX * TILE_SIZE;
    player.lastDrawnPixelY = PLAY_AREA_Y + player.tileY * TILE_SIZE;

    player.pixelX = player.tileX * TILE_SIZE;
    player.pixelY = player.tileY * TILE_SIZE;
    player.velY = 0;
    player.grounded = true;
    player.cameraX = 0;

    projectilesInit();
    inventoryInit(&player.inv);
    inventoryAdd(&player.inv, INV_FOOD, 5);
    inventoryAdd(&player.inv, INV_ARMOR, 1);

    randomSeed(analogRead(A0) ^ (uint32_t)micros());
    worldmapInit();
    enemyInit(testMap, player.tileX, player.tileY, AREA_INDUSTRIAL);
    weaponInit(&player.weapon);
    characterInit();

    inventoryVisible = false;
    enemiesMoved = false;
    enemyKilledFlag = false;
    statusBarDirty = false;
    clearMedkitSpawned = false;
    portalJustAppeared = false;
    altarUnlockPlaying = false;
    joySwitchReleased = true;
    tradeMenuOpen = false;
    portalReset();

    currentState = STATE_BOOT;
    bootTimer = millis();
}

// World map input logic
static void updateWorldMap() {
    static bool lastAPress = false;
    static uint32_t enterTime = 0;

    if (enterTime == 0) enterTime = millis();
    bool currentAPress = isButtonAPressed();
    worldmapUpdate();

    if (currentAPress && !lastAPress && millis() - enterTime > 250) {
        int area = worldmapGetSelectedArea();
        worldmapLoadCurrentAreaMap(testMap);
        worldmapResetPlayerPosition(testMap, &player.tileX, &player.tileY);

        player.moveDirX = 0;
        player.moveDirY = 0;
        player.moveProgress = 0;
        player.isMoving = false;
        player.pixelX = player.tileX * TILE_SIZE;
        player.pixelY = player.tileY * TILE_SIZE;

        if (worldmapIsPlatformerArea(area)) {
            player.velY = 0;
            player.grounded = true;
            player.cameraX = (player.pixelX / PLAY_AREA_W) * PLAY_AREA_W;
        } else {
            player.cameraX = 0;
        }

        player.lastDrawnPixelX = PLAY_AREA_X + player.tileX * TILE_SIZE;
        player.lastDrawnPixelY = PLAY_AREA_Y + player.tileY * TILE_SIZE;

        uiSetArea(area);
        uiSetCameraX(player.cameraX);
        dropInitForArea(testMap, area);
        portalReset();

        if (area == AREA_HOME || area == AREA_TRADE || area == AREA_ALTAR) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                enemies[i].active = false;
                enemies[i].pendingDeathClear = false;
            }
        } else {
            enemyInit(testMap, player.tileX, player.tileY, area);
        }

        portalSpawn(testMap);
        currentState = STATE_EXPLORE;
        joySwitchReleased = true;
        tradeMenuOpen = false;
        redrawRequired = true;
        projectilesInit();
        enemyKilledFlag = false;
        enemiesMoved = false;
        portalJustAppeared = false;
        clearMedkitSpawned = false;
        enterTime = 0;
    }
    lastAPress = currentAPress;
}

// Main game update loop
void gameUpdate() {
    updateInput();

    switch (currentState) {
        case STATE_BOOT:     updateBootScreen();   break;
        case STATE_CG:       updateCutscene();     break;
        case STATE_CG2:      updateFinalCutscene();break;
        case STATE_MENU:     updateMainMenu();     break;
        case STATE_EXPLORE:  updateExploration();  break;
        case STATE_WORLDMAP: updateWorldMap();      break;
    }
}

// Main game render loop
void gameRender() {
    if (currentState != lastState) {
        redrawRequired = true;
        lastState = currentState;
    }

    switch (currentState) {
        case STATE_BOOT:
            if (redrawRequired) {
                drawBootScreen();
                redrawRequired = false;
            }
            break;

        case STATE_CG:
            if (redrawRequired || cgNeedsRedraw()) {
                cgDraw();
                redrawRequired = false;
            }
            break;

        case STATE_CG2:
            cg2Draw();
            redrawRequired = false;
            break;

        case STATE_MENU:
            if (redrawRequired) {
                drawMainMenu();
                redrawRequired = false;
            }
            break;

        case STATE_EXPLORE: {
            static bool lastInv = false;
            static bool lastTrade = false;
            static int lastTradeIdx = 0;
            static int lastAltarTalk = 0;
            static bool showOffer = false;
            static uint32_t altarCooldown = 0;
            static int lastSupplyTalk = 0;
            static bool showMerchant = false;
            static uint32_t supplyCooldown = 0;
            static bool miracleShown = false;

            bool altarDirty = false;
            bool supplyDirty = false;

            if (altarUnlockPlaying) {
                uint32_t elapsed = millis() - altarUnlockTimer;
                drawAltarWeaponRise(testMap, &player, elapsed);
                updateLaserRiseSound(elapsed);
                if (!miracleShown) {
                    drawSkullMiracleDialogue();
                    miracleShown = true;
                }
                redrawRequired = false;
                break;
            }
            miracleShown = false;

            if (redrawRequired) {
                drawExploreScreen(testMap, &player);
                latchEntityDrawPositions(&player);
                lastPlayerTileX = player.tileX;
                lastPlayerTileY = player.tileY;
                redrawRequired = false;
                statusBarDirty = false;
                enemiesMoved = false;
                lastInv = false;
                lastTrade = false;
                lastTradeIdx = tradeMenuIndex;
                lastAltarTalk = 0;
                showOffer = false;
                altarCooldown = 0;
                lastSupplyTalk = 0;
                showMerchant = false;
                supplyCooldown = 0;
                altarDirty = true;
                supplyDirty = true;
            } else if (inventoryVisible && !lastInv) {
                drawInventoryOverlay(&player);
                lastInv = true;
            } else if (tradeMenuOpen && !lastTrade) {
                drawSupplyTradeOverlay(&player, tradeMenuIndex, tradeMessage, tradeMessageIsError);
                lastTrade = true;
                lastTradeIdx = tradeMenuIndex;
                tradeMenuDirty = false;
            } else if (!inventoryVisible && lastInv) {
                drawPlayArea(testMap, &player);
                latchEntityDrawPositions(&player);
                lastPlayerTileX = player.tileX;
                lastPlayerTileY = player.tileY;
                enemiesMoved = false;
                lastInv = false;
                lastTrade = false;
                showMerchant = false;
                altarDirty = true;
                supplyDirty = true;
            } else if (!tradeMenuOpen && lastTrade) {
                drawPlayArea(testMap, &player);
                latchEntityDrawPositions(&player);
                lastPlayerTileX = player.tileX;
                lastPlayerTileY = player.tileY;
                enemiesMoved = false;
                lastTrade = false;
                showMerchant = false;
                altarDirty = true;
                supplyDirty = true;
            } else if (tradeMenuOpen && tradeMenuDirty) {
                if (!tradeMessage && lastTradeIdx != tradeMenuIndex) {
                    drawSupplyTradeSelection(&player, lastTradeIdx, tradeMenuIndex);
                } else {
                    drawSupplyTradeOverlay(&player, tradeMenuIndex, tradeMessage, tradeMessageIsError);
                }
                lastTradeIdx = tradeMenuIndex;
                tradeMenuDirty = false;
            } else if (enemyKilledFlag && !inventoryVisible) {
                drawPendingEnemyDeaths(testMap);
                if (clearMedkitSpawned) {
                    drawEnemyDeath(testMap, clearMedkitTileX, clearMedkitTileY);
                    clearMedkitSpawned = false;
                }
                if (portalJustAppeared) {
                    drawPortal();
                    portalJustAppeared = false;
                }
                enemyKilledFlag = false;
                enemiesMoved = false;
                drawProjectilesUpdate(testMap, &player);
                drawSmoothEntitiesUpdate(testMap, &player);
            } else if (!inventoryVisible) {
                drawProjectilesUpdate(testMap, &player);
                drawSmoothEntitiesUpdate(testMap, &player);
                enemiesMoved = false;
            }
            lastPlayerTileX = player.tileX;
            lastPlayerTileY = player.tileY;

            if (statusBarDirty && !inventoryVisible && !tradeMenuOpen) {
                drawStatusBar(&player);
                statusBarDirty = false;
            }

            if (!inventoryVisible && !tradeMenuOpen && worldmapGetSelectedArea() == AREA_ALTAR) {
                int raw = 0;
                bool offerPrompt = altarNearOffering(player.tileX, player.tileY);
                bool hasRelics = player.inv.alienRelics > 0;

                if (altarNearPriest(player.tileX, player.tileY))  raw = 1;
                if (altarNearSkull(player.tileX, player.tileY))   raw = 2;

                int speaker = raw;
                if (lastAltarTalk == 0 && millis() < altarCooldown) speaker = 0;

                if (speaker == 0 && lastAltarTalk != 0) {
                    clearAltarDialogueRegion(testMap, &player);
                    altarCooldown = millis() + 1200;
                    altarDirty = true;
                }
                if (speaker != 0 && lastAltarTalk != 0 && speaker != lastAltarTalk) {
                    clearAltarDialogueRegion(testMap, &player);
                    altarDirty = true;
                }
                if (offerPrompt != showOffer) {
                    clearAltarDialogueRegion(testMap, &player);
                    altarDirty = true;
                }

                if (speaker == 1 && (altarDirty || lastAltarTalk != 1)) drawAltarDialogue();
                if (speaker == 2 && (altarDirty || lastAltarTalk != 2)) drawSkullDialogue();
                if (offerPrompt && (altarDirty || !showOffer)) drawOfferRelicsPrompt(hasRelics);

                lastAltarTalk = speaker;
                showOffer = offerPrompt;
                lastSupplyTalk = 0;
            } else {
                lastAltarTalk = 0;
                showOffer = false;

                if (!inventoryVisible && !tradeMenuOpen && worldmapGetSelectedArea() == AREA_TRADE) {
                    int raw = 0;
                    bool merchantPrompt = supplyNearMerchant(player.tileX, player.tileY);

                    if (supplyNearWarrior(player.tileX, player.tileY))   raw = 1;
                    if (supplyNearCivilian(player.tileX, player.tileY))  raw = 2;
                    if (supplyNearWizard(player.tileX, player.tileY))    raw = 3;

                    int speaker = raw;
                    if (lastSupplyTalk == 0 && millis() < supplyCooldown) speaker = 0;

                    if (speaker == 0 && lastSupplyTalk != 0) {
                        clearSupplyDialogueRegion(testMap, &player);
                        supplyCooldown = millis() + 1200;
                        supplyDirty = true;
                    }
                    if (speaker != 0 && lastSupplyTalk != 0 && speaker != lastSupplyTalk) {
                        clearSupplyDialogueRegion(testMap, &player);
                        supplyDirty = true;
                    }
                    if (merchantPrompt != showMerchant) {
                        clearSupplyDialogueRegion(testMap, &player);
                        supplyDirty = true;
                    }

                    if (speaker == 1 && (supplyDirty || lastSupplyTalk != 1)) drawSupplyWarriorDialogue();
                    if (speaker == 2 && (supplyDirty || lastSupplyTalk != 2)) drawSupplyCivilianDialogue();
                    if (speaker == 3 && (supplyDirty || lastSupplyTalk != 3)) drawSupplyWizardDialogue();
                    if (merchantPrompt && (supplyDirty || !showMerchant)) drawSupplyTradePrompt();

                    lastSupplyTalk = speaker;
                    showMerchant = merchantPrompt;
                } else {
                    lastSupplyTalk = 0;
                    showMerchant = false;
                }
            }
            break;
        }

        case STATE_WORLDMAP:
            if (redrawRequired) {
                worldmapDraw();
                redrawRequired = false;
            }
            break;
    }
}