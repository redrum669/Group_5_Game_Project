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

static GameState gState        = STATE_BOOT;
static GameState prevState     = STATE_BOOT;
static Player    player;

static uint32_t  bootStart     = 0;
static bool      screenDirty   = true;
static int       prevPlayerX   = -1;
static int       prevPlayerY   = -1;
static bool      inventoryOpen = false;  // overlay toggle
static bool      enemiesMoved       = false;  // triggers incremental enemy redraw
static bool      enemyDied          = false;  // triggers tile redraw at kill position
static bool      statsDirty         = false;  // triggers status-bar redraw
static bool      boardClearSpawned  = false;  // a board-clear medkit was just placed
static int       boardClearX        = -1;     // tile X of board-clear medkit
static int       boardClearY        = -1;     // tile Y of board-clear medkit
static bool      portalJustSpawned  = false;  // portal needs to be drawn this frame
static bool      altarUnlockAnimating = false;
static uint32_t  altarUnlockStarted   = 0;
static bool      joySwNeedsRelease    = true;
static bool      tradeOpen            = false;
static bool      tradeDirty           = false;
static int       tradeSelection       = 0;
static const char* tradeNotice        = "";
static bool      tradeNoticeError     = false;

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

// 0 empty  1 wall

static bool isBlocked(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= MAP_COLS || ty >= MAP_ROWS) return true;

    // Restrict safe areas to 14 columns (single screen)
    if (!worldmapIsPlatformerArea(worldmapGetSelectedArea()) && tx >= 14) return true;

    if (testMap[ty][tx] == 1) return true;  // wall
    if (worldmapGetSelectedArea() == AREA_TRADE && supplyIsSolidEntity(tx, ty)) return true;
    if (enemyAt(tx, ty)) return true;        // enemy occupies tile
    return false;
}


static bool checkMapCollision(int px, int py, int w, int h) {
    int tx1 = px / TILE_SIZE;
    int ty1 = py / TILE_SIZE;
    int tx2 = (px + w - 1) / TILE_SIZE;
    int ty2 = (py + h - 1) / TILE_SIZE;

    if (tx1 < 0) tx1 = 0;
    if (tx2 >= MAP_COLS) tx2 = MAP_COLS - 1;

    for (int y = ty1; y <= ty2; y++) {
        for (int x = tx1; x <= tx2; x++) {
            if (y >= 0 && y < MAP_ROWS && x >= 0 && x < MAP_COLS) {
                if (testMap[y][x] == 1) return true;
            }
        }
    }
    return false;
}

// Latch every entity's lastDrawn pixel pos to its current tile origin.
// Call after any full-area redraw (drawExploreScreen / drawPlayArea) so the
// next drawSmoothEntitiesUpdate sees a correct baseline and can erase cleanly.
// Also snaps in-flight interpolation so the player does not visually jump
// between a tile-pos draw and a lerp-pos draw on the next frame.
static void latchEntityDrawPositions(Player* p) {
    p->moveDirX        = 0;
    p->moveDirY        = 0;
    p->moveProgress    = 0;
    p->isMoving        = false;

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
        enemies[i].moveDirX        = 0;
        enemies[i].moveDirY        = 0;
        enemies[i].moveProgress    = 0;
        enemies[i].isMoving        = false;

        if (isPlatformer) {
            enemies[i].lastDrawnPixelX = PLAY_AREA_X + enemies[i].pixelX - p->cameraX;
            enemies[i].lastDrawnPixelY = PLAY_AREA_Y + enemies[i].pixelY;
        } else {
            enemies[i].lastDrawnPixelX = PLAY_AREA_X + enemies[i].tileX * TILE_SIZE;
            enemies[i].lastDrawnPixelY = PLAY_AREA_Y + enemies[i].tileY * TILE_SIZE;
        }
    }
}

static bool areaUsesBoardClearMedkit() {
    int area = worldmapGetSelectedArea();
    return area != AREA_HOME &&
           area != AREA_TRADE &&
           area != AREA_ALTAR;
}

static bool performSupplyTrade() {
    switch (tradeSelection) {
        case 0:
            if (player.inv.seeds < 3) {
                tradeNotice = "Need 3 Seeds";
                tradeNoticeError = true;
                return false;
            }
            inventoryAdd(&player.inv, INV_SEEDS, -3);
            inventoryAdd(&player.inv, INV_FOOD, 1);
            tradeNotice = "Traded Seeds for Food";
            tradeNoticeError = false;
            return true;

        case 1:
            if (player.inv.metal < 4) {
                tradeNotice = "Need 4 Metal";
                tradeNoticeError = true;
                return false;
            }
            inventoryAdd(&player.inv, INV_METAL, -4);
            inventoryAdd(&player.inv, INV_FOOD, 2);
            tradeNotice = "Traded Metal for Rations";
            tradeNoticeError = false;
            return true;

        case 2:
            if (player.inv.alienRelics < 2) {
                tradeNotice = "Need 2 Relics";
                tradeNoticeError = true;
                return false;
            }
            inventoryAdd(&player.inv, INV_ALIEN_RELICS, -2);
            inventoryAdd(&player.inv, INV_METAL, 3);
            tradeNotice = "Traded Relics for Metal";
            tradeNoticeError = false;
            return true;

        default:
            if (player.inv.food < 6) {
                tradeNotice = "Need 6 Food";
                tradeNoticeError = true;
                return false;
            }
            inventoryAdd(&player.inv, INV_FOOD, -6);
            player.hp = 100;
            statsDirty = true;
            tradeNotice = "Blood pack restored all HP";
            tradeNoticeError = false;
            return true;
    }
}

static void updateBoot() {
    if (millis() - bootStart > 200) {
        cgInit();
        gState = STATE_CG;
    }
}

static void updateCG() {
    cgDrawUpdate();          // runs typewriter + blood drip animation every frame
    if (cgUpdate()) {
        gState = STATE_MENU;
    }
}

static void updateCG2() {
    cg2DrawUpdate();
    if (cg2Update()) {
        gState = STATE_MENU;
    }
}

static void updateMenu() {
    static bool prevA = false;
    bool curA = isButtonAPressed();
    if (curA && !prevA) {
        Serial.println("STOP_MUSIC");
        gState = STATE_WORLDMAP;
        screenDirty = true;
    }
    prevA = curA;
}


static void updateExplore() {
    static uint32_t lastMove       = 0;
    static uint32_t lastHungerTick = 0;
    static uint32_t lastStarvTick  = 0;
    static uint32_t lastJoyToggleMs = 0;
    static uint32_t lastTradeMoveMs = 0;
    static bool prevBtnA       = false;
    static bool prevBtnB       = false;
    static bool prevBtnC       = false;
    static bool joySwLatched   = false;
    static bool prevNearPortal = false;
    static bool prevTradeUp    = false;
    static bool prevTradeDown  = false;

    // ---- JOY_SW debounced press: toggle inventory overlay (always active) ----
    bool curJoySW = isJoySWPressed();
    bool curBtnA = isButtonAPressed();
    bool curBtnC = isButtonCPressed();
    if (joySwNeedsRelease) {
        if (!curJoySW) {
            joySwNeedsRelease = false;
            joySwLatched = false;
        }
    } else if (!curJoySW) {
        joySwLatched = false;
    } else if (!joySwLatched && millis() - lastJoyToggleMs > 180) {
        if (tradeOpen) {
            tradeOpen = false;
            tradeDirty = false;
        } else {
            inventoryOpen = !inventoryOpen;
        }
        joySwLatched = true;
        lastJoyToggleMs = millis();
    }

    // ---- BTN_B edge: dual function depending on inventory state ----
    bool curBtnB = isButtonBPressed();
    if (curBtnB && !prevBtnB) {
        if (tradeOpen) {
            performSupplyTrade();
            tradeDirty = true;
        } else if (inventoryOpen) {
            // Inventory open → eat food
            if (player.inv.food > 0) {
                inventoryAdd(&player.inv, INV_FOOD, -1);
                player.hunger += FOOD_HUNGER_RESTORE;
                if (player.hunger > 100) player.hunger = 100;
                // Targeted updates — no full panel or play-area redraw
                drawInventoryFoodCount(player.inv.food); // update food qty in overlay
                drawStatusBar(&player);                  // update hunger bar immediately
            }
        } else {
            // Inventory closed → switch weapon
            bool nearMerchant =
                worldmapGetSelectedArea() == AREA_TRADE &&
                supplyNearMerchant(player.tileX, player.tileY);
            bool nearOfferingAltar =
                worldmapGetSelectedArea() == AREA_ALTAR &&
                altarNearOffering(player.tileX, player.tileY);
            bool altarCanUnlockWeapon = nearOfferingAltar;
            bool canOfferRelics =
                altarCanUnlockWeapon &&
                player.inv.alienRelics > 0;

            if (nearMerchant) {
                tradeOpen = true;
                tradeSelection = 0;
                tradeNotice = "";
                tradeNoticeError = false;
                tradeDirty = true;
            } else if (canOfferRelics) {
                inventoryAdd(&player.inv, INV_ALIEN_RELICS, -player.inv.alienRelics);
                weaponUnlock(&player.weapon, WEAPON_LASER);
                weaponUnlock(&player.weapon, WEAPON_LASER_GUN);
                player.weapon.type = WEAPON_LASER_GUN;
                altarUnlockAnimating = true;
                altarUnlockStarted = millis();
                screenDirty = true;
            } else if (altarCanUnlockWeapon) {
                screenDirty = true;
            } else {
                weaponSwitch(&player.weapon);
                playWeaponSwapSound();
                drawWeaponSlot(player.weapon.type);
            }
        }
    }
    prevBtnB = curBtnB;

    if (tradeOpen && curBtnA && !prevBtnA) {
        tradeOpen = false;
        tradeDirty = false;
        prevBtnA = curBtnA;
        return;
    }

    if (tradeOpen) {
        bool curUp = isUpPressed();
        bool curDown = isDownPressed();

        if (millis() - lastTradeMoveMs > 160) {
            if (curUp && !prevTradeUp) {
                tradeSelection--;
                if (tradeSelection < 0) tradeSelection = 3;
                tradeNotice = "";
                tradeNoticeError = false;
                tradeDirty = true;
                lastTradeMoveMs = millis();
            } else if (curDown && !prevTradeDown) {
                tradeSelection++;
                if (tradeSelection > 3) tradeSelection = 0;
                tradeNotice = "";
                tradeNoticeError = false;
                tradeDirty = true;
                lastTradeMoveMs = millis();
            }
        }

        prevTradeUp = curUp;
        prevTradeDown = curDown;
        prevBtnA = curBtnA;
        return;
    }

    // ---- PAUSE: while inventory is open nothing else runs ----
    if (inventoryOpen) {
        if (curBtnC && !prevBtnC) {
            if (player.inv.armor > 0) {
                inventoryAdd(&player.inv, INV_ARMOR, -1);
                player.armor += ARMOR_RESTORE;
                if (player.armor > 100) player.armor = 100;
                drawInventoryArmorCount(player.inv.armor);
                drawStatusBar(&player);
            }
        }
        prevBtnC = curBtnC;
        return;
    }

    if (altarUnlockAnimating) {
        if (millis() - altarUnlockStarted >= 7200) {
            altarUnlockAnimating = false;
            screenDirty = true;
            drawWeaponSlot(player.weapon.type);
        }
        return;
    }

    // ---- Hunger drain over time (very slow) ----
    if (millis() - lastHungerTick >= HUNGER_DRAIN_INTERVAL) {
        lastHungerTick = millis();
        if (player.hunger > 0) {
            player.hunger--;
            statsDirty = true;
        }
    }

    // ---- Starvation: lose HP when hunger == 0 ----
    if (player.hunger == 0 && millis() - lastStarvTick >= STARVATION_INTERVAL) {
        lastStarvTick = millis();
        player.hp -= STARVATION_DAMAGE;
        if (player.hp < 0) player.hp = 0;
        statsDirty = true;
    }

    // ---- Enemy AI update ----
    int prevHp = player.hp;
    int prevArmor = player.armor;
    if (enemyUpdate(testMap, &player)) {
        enemiesMoved = true;
    }
    if (player.hp != prevHp || player.armor != prevArmor) {
        statsDirty = true;  // enemy hit us — refresh status bar
    }

    // ---- Laser alien: draw beam flash for any bolt fired this tick ----
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        if (enemies[i].laserFired) {
            drawLaserFlash(testMap,
                           enemies[i].tileX,    enemies[i].tileY,
                           enemies[i].laserEndX, enemies[i].laserEndY,
                           &player);
            enemies[i].laserFired = false;
        }
    }

    // ---- BTN_A edge: enter portal if nearby, or trigger CG2 in HOME ----
    if (curBtnA && !prevBtnA) {
        if (portal.active && portalNearPlayer(player.tileX, player.tileY)) {
            gState = STATE_WORLDMAP;
            screenDirty = true;
        } else if (worldmapGetSelectedArea() == AREA_HOME) {
            cg2Init();
            gState = STATE_CG2;
            screenDirty = true;
        }
    }
    prevBtnA = curBtnA;

    // ---- BTN_C edge: fire current weapon (rate limited inside weaponFire) ----
    bool isPlatformer = worldmapIsPlatformerArea(worldmapGetSelectedArea());

    if (player.weapon.type == WEAPON_LASER_GUN) {
        if (curBtnC) {
            if (!player.weapon.isCharging) {
                player.weapon.isCharging = true;
                player.weapon.chargeStartMs = millis();
            }
            updateChargingSound(true, player.weapon.chargeStartMs);
        } else {
            if (player.weapon.isCharging) {
                uint32_t chargeTime = millis() - player.weapon.chargeStartMs;
                player.weapon.isCharging = false;
                updateChargingSound(false, 0);

                if (chargeTime >= 1000) {
                    // Fire laser cannon
                    player.weapon.lastFire = millis();
                    enemyAlertAll();
                    playWeaponFire(WEAPON_LASER_GUN);

                    int dir = characterGetDir();
                    if (isPlatformer && dir != CHAR_DIR_LEFT) dir = CHAR_DIR_RIGHT; // Prevent shooting UP/DOWN in platformer

                    int pMinX = player.pixelX, pMaxX = player.pixelX + 15;
                    int pMinY = player.pixelY, pMaxY = player.pixelY + 15;

                    if (isPlatformer) {
                        if (dir == CHAR_DIR_LEFT) { pMinX = 0; pMaxX = player.pixelX; }
                        else { pMinX = player.pixelX; pMaxX = MAP_COLS * TILE_SIZE; }
                    } else {
                        if (dir == CHAR_DIR_LEFT) { pMinX = 0; pMaxX = player.pixelX; }
                        else if (dir == CHAR_DIR_RIGHT) { pMinX = player.pixelX; pMaxX = MAP_COLS * TILE_SIZE; }
                        else if (dir == CHAR_DIR_UP) { pMinY = 0; pMaxY = player.pixelY; }
                        else { pMinY = player.pixelY; pMaxY = MAP_ROWS * TILE_SIZE; }
                    }

                    bool hitAnything = false;
                    for (int e = 0; e < MAX_ENEMIES; e++) {
                        if (enemies[e].active) {
                            int ex = enemies[e].pixelX + 8;
                            int ey = enemies[e].pixelY + 8;
                            if (ex >= pMinX && ex <= pMaxX && ey >= pMinY && ey <= pMaxY) {
                                enemies[e].hp = 0;
                                dropAdd(enemies[e].tileX, enemies[e].tileY, INV_ALIEN_RELICS);
                                enemies[e].pendingDeathClear = false; // no need, laser flash will erase everything cleanly
                                enemies[e].active = false;
                                hitAnything = true;
                            }
                        }
                    }

                    int recoilX = 0, recoilY = 0;
                    if (dir == CHAR_DIR_LEFT) recoilX = 16;
                    else if (dir == CHAR_DIR_RIGHT) recoilX = -16;
                    else if (!isPlatformer && dir == CHAR_DIR_UP) recoilY = 16;
                    else if (!isPlatformer && dir == CHAR_DIR_DOWN) recoilY = -16;

                    int originalX = player.pixelX;
                    int originalY = player.pixelY;

                    player.pixelX += recoilX;
                    player.pixelY += recoilY;
                    if (player.pixelX < 0) player.pixelX = 0;
                    int maxPx = MAP_COLS * TILE_SIZE - TILE_SIZE;
                    if (player.pixelX > maxPx) player.pixelX = maxPx;
                    if (player.pixelY < 0) player.pixelY = 0;
                    int maxPy = MAP_ROWS * TILE_SIZE - TILE_SIZE;
                    if (player.pixelY > maxPy) player.pixelY = maxPy;

                    if (checkMapCollision(player.pixelX, player.pixelY, TILE_SIZE, TILE_SIZE)) {
                        if (recoilX > 0) player.pixelX = ((player.pixelX + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
                        else if (recoilX < 0) player.pixelX = (player.pixelX / TILE_SIZE) * TILE_SIZE + TILE_SIZE;

                        if (recoilY > 0) player.pixelY = ((player.pixelY + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
                        else if (recoilY < 0) player.pixelY = (player.pixelY / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
                    }

                    int actualRecoilX = player.pixelX - originalX;
                    int actualRecoilY = player.pixelY - originalY;

                    player.pixelX = originalX;
                    player.pixelY = originalY;

                    drawLaserCannonFlash(testMap, player.tileX, player.tileY, dir, isPlatformer, &player, actualRecoilX, actualRecoilY);

                    player.pixelX = originalX + actualRecoilX;
                    player.pixelY = originalY + actualRecoilY;
                    player.tileX = (player.pixelX + 8) / TILE_SIZE;
                    player.tileY = (player.pixelY + 8) / TILE_SIZE;

                    screenDirty = true; // force a redraw after recoil
                }
            }
        }
    } else {
        if (curBtnC && !prevBtnC) {
            int fired = weaponFire(&player.weapon, player.pixelX, player.pixelY, characterGetDir(), isPlatformer);
            if (fired) {
                playWeaponFire(player.weapon.type);
            }
            if (fired && isPlatformer && player.weapon.type == WEAPON_LASER) {
                // Slight dash forward for physical swing feedback!
                int dashDir = (characterGetDir() == CHAR_DIR_LEFT) ? -8 : 8;
                player.pixelX += dashDir;
                if (player.pixelX < 0) player.pixelX = 0;
                int maxPx = MAP_COLS * TILE_SIZE - TILE_SIZE;
                if (player.pixelX > maxPx) player.pixelX = maxPx;

                if (checkMapCollision(player.pixelX, player.pixelY, TILE_SIZE, TILE_SIZE)) {
                    if (dashDir > 0) player.pixelX = ((player.pixelX + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
                    else             player.pixelX = (player.pixelX / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
                }
            }
        }
    }
    prevBtnC = curBtnC;

    // Update projectiles
    projectilesUpdate(testMap);

    // Check if enemies died (e.g. from projectiles)
    bool anyEnemyDied = false;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active && enemies[i].pendingDeathClear) {
            anyEnemyDied = true;
            // Let the renderer clear it first, but we know it died
        }
    }
    if (anyEnemyDied && !enemyDied) {
        enemyDied  = true;
        statsDirty = true;
        // Spawn portal after all enemies are dead (only once)
        if (enemyAllDead() && !portal.active) {
            if (areaUsesBoardClearMedkit() &&
                dropAddRandom(INV_MEDKIT, testMap, &boardClearX, &boardClearY)) {
                boardClearSpawned = true;
            }
            if (portalSpawn(testMap)) {
                portalJustSpawned = true;
            }
        }
    }

    // ---- Smooth pixel-level movement ----
    // While isMoving, advance moveProgress each frame. When the 16-px step
    // completes, commit the tile change and (if the stick is still held in
    // a valid direction) start the next step immediately — no cooldown gap.
    (void)lastMove; // legacy variable, no longer gating movement

    if (isPlatformer) {
        // ---- Platformer physics (Combat Areas) ----
        int dir = -1;
        int dx = 0;
        if (isLeftPressed()) {
            dx = -PLAYER_MOVE_PX;
            dir = CHAR_DIR_LEFT;
        } else if (isRightPressed()) {
            dx = PLAYER_MOVE_PX;
            dir = CHAR_DIR_RIGHT;
        }

        if (dx != 0) {
            player.pixelX += dx;
            if (player.pixelX < 0) player.pixelX = 0;
            int maxPx = MAP_COLS * TILE_SIZE - TILE_SIZE;
            if (player.pixelX > maxPx) player.pixelX = maxPx;

            if (checkMapCollision(player.pixelX, player.pixelY, TILE_SIZE, TILE_SIZE)) {
                if (dx > 0) player.pixelX = ((player.pixelX + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
                else        player.pixelX = (player.pixelX / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
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
        if (player.velY > 8) player.velY = 8; // terminal velocity

        player.pixelY += player.velY;
        player.grounded = false;

        // Ground collision
        if (checkMapCollision(player.pixelX, player.pixelY, TILE_SIZE, TILE_SIZE)) {
            if (player.velY > 0) { // landing
                player.pixelY = ((player.pixelY + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE - TILE_SIZE;
                player.grounded = true;
            } else if (player.velY < 0) { // hitting ceiling
                player.pixelY = (player.pixelY / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
            }
            player.velY = 0;
        }

        // Death pit
        if (player.pixelY > MAP_ROWS * TILE_SIZE + TILE_SIZE) {
            player.hp = 0;
            statsDirty = true;
        }

        if (dir != -1) characterSetDir(dir);

        // Logical tile update
        int oldTileX = player.tileX;
        int oldTileY = player.tileY;
        player.tileX = (player.pixelX + TILE_SIZE / 2) / TILE_SIZE;
        player.tileY = (player.pixelY + TILE_SIZE / 2) / TILE_SIZE;

        // Pickup items only if we enter a new tile (simplification)
        if (player.tileX != oldTileX || player.tileY != oldTileY) {
            int picked = dropPickup(player.tileX, player.tileY, &player.inv);
            if (picked == INV_MEDKIT) {
                player.hp += MEDKIT_HEAL;
                if (player.hp > 100) player.hp = 100;
                statsDirty = true;
            }
        }

        // Camera update (Room-based scrolling to avoid full-screen SPI flicker)
        int roomWidth = PLAY_AREA_W;
        int newCameraX = (player.pixelX / roomWidth) * roomWidth;

        // Clamp camera to map bounds
        int maxCameraX = (MAP_COLS * TILE_SIZE / roomWidth) * roomWidth - roomWidth;
        if (maxCameraX < 0) maxCameraX = 0;
        if (newCameraX > maxCameraX) newCameraX = maxCameraX;

        if (newCameraX != player.cameraX) {
            player.cameraX = newCameraX;
            // Force a full redraw of the play area when moving to a new room
            extern bool screenDirty;
            screenDirty = true;
            projectilesInit();
        }

    } else {
        // ---- Top-down grid movement (Safe Areas) ----
        if (player.isMoving) {
            player.moveProgress += PLAYER_MOVE_PX;
            if (player.moveProgress >= TILE_SIZE) {
                // Commit step.
                player.tileX += player.moveDirX;
                player.tileY += player.moveDirY;
                player.moveDirX = 0;
                player.moveDirY = 0;
                player.moveProgress = 0;
                player.isMoving = false;

                // Pickup whatever sits on the new tile.
                int picked = dropPickup(player.tileX, player.tileY, &player.inv);
                if (picked == INV_MEDKIT) {
                    player.hp += MEDKIT_HEAL;
                    if (player.hp > 100) player.hp = 100;
                    statsDirty = true;
                }
            }
        }

        // Read current stick direction (may start a new step if not moving).
        if (!player.isMoving) {
            int ndx = 0, ndy = 0;
            int dir = -1;
            if      (isUpPressed())    { ndy = -1; dir = CHAR_DIR_UP;    }
            else if (isDownPressed())  { ndy =  1; dir = CHAR_DIR_DOWN;  }
            else if (isLeftPressed())  { ndx = -1; dir = CHAR_DIR_LEFT;  }
            else if (isRightPressed()) { ndx =  1; dir = CHAR_DIR_RIGHT; }

            if (ndx != 0 || ndy != 0) {
                int nx = player.tileX + ndx;
                int ny = player.tileY + ndy;
                if (!isBlocked(nx, ny)) {
                    player.moveDirX = (int8_t)ndx;
                    player.moveDirY = (int8_t)ndy;
                    player.moveProgress = 0;
                    player.isMoving = true;
                    characterSetDir(dir);
                }
            }
        }

        // Sync pixel coords for UI / rendering consistency
        player.pixelX = player.tileX * TILE_SIZE + player.moveDirX * player.moveProgress;
        player.pixelY = player.tileY * TILE_SIZE + player.moveDirY * player.moveProgress;
        player.cameraX = 0; // No scrolling in safe areas
    }

    // Sync camera offset to the renderer
    uiSetCameraX(player.cameraX);

    // Portal prompt follows the logical tile.
    bool nearPortal = portalNearPlayer(player.tileX, player.tileY);
    if (nearPortal != prevNearPortal) {
        drawPortalSlot(nearPortal);
        prevNearPortal = nearPortal;
    }
}

void gameInit() {
    // Send stop music to ensure it's quiet on boot
    Serial.println("STOP_MUSIC");
    bootStart     = millis(); // Ensure boot timing is always anchored to initialization time
    player.tileX  = 2;
    player.tileY  = 2;
    player.hp     = 100;
    player.hunger = 100;
    player.armor  = 100;
    // Smooth-movement state
    player.moveDirX        = 0;
    player.moveDirY        = 0;
    player.moveProgress    = 0;
    player.isMoving        = false;
    player.lastDrawnPixelX = PLAY_AREA_X + player.tileX * TILE_SIZE;
    player.lastDrawnPixelY = PLAY_AREA_Y + player.tileY * TILE_SIZE;

    // Platformer physics state
    player.pixelX          = player.tileX * TILE_SIZE;
    player.pixelY          = player.tileY * TILE_SIZE;
    player.velY            = 0;
    player.grounded        = true;
    player.cameraX         = 0;

    projectilesInit();

    // Initialise inventory. Resource pickups start from zero so map loot
    // changes are easy to see in the backpack counts.
    inventoryInit(&player.inv);
    inventoryAdd(&player.inv, INV_FOOD,          5);
    inventoryAdd(&player.inv, INV_ARMOR,         1);

    // Seed RNG from floating analog pin noise + timing
    randomSeed(analogRead(A0) ^ (uint32_t)micros());
    worldmapInit();   // loads all area maps; drops are placed per-area when entering
    enemyInit(testMap, player.tileX, player.tileY, AREA_INDUSTRIAL); // default combat setup
    weaponInit(&player.weapon);                      // default weapon: SMG
    projectilesInit();
    characterInit();                                 // reset sprite direction + frame

    inventoryOpen      = false;
    enemiesMoved       = false;
    enemyDied          = false;
    statsDirty         = false;
    boardClearSpawned  = false;
    boardClearX        = -1;
    boardClearY        = -1;
    portalJustSpawned  = false;
    altarUnlockAnimating = false;
    altarUnlockStarted   = 0;
    joySwNeedsRelease    = true;
    tradeOpen            = false;
    tradeDirty           = false;
    tradeSelection       = 0;
    tradeNotice          = "";
    tradeNoticeError     = false;
    portalReset();
    gState        = STATE_BOOT;
    bootStart     = millis();
}
// ------------------------------------------------------------------
// updateWorldMap: handle input while showing world map
// ------------------------------------------------------------------
static void updateWorldMap() {
    static bool     prevA     = false;
    static uint32_t enteredAt = 0;

    // Record when we first arrive so A held from previous screen doesn't pass through
    if (enteredAt == 0) enteredAt = millis();

    bool curA = isButtonAPressed();
    worldmapUpdate();  // incremental icon redraw handled internally

    if (curA && !prevA && millis() - enteredAt > 250) {
        int area = worldmapGetSelectedArea();
        worldmapLoadCurrentAreaMap(testMap);
        worldmapResetPlayerPosition(testMap, &player.tileX, &player.tileY);
        // Reset smooth-movement state so player snaps cleanly to spawn tile.
        player.moveDirX        = 0;
        player.moveDirY        = 0;
        player.moveProgress    = 0;
        player.isMoving        = false;
        player.pixelX          = player.tileX * TILE_SIZE;
        player.pixelY          = player.tileY * TILE_SIZE;
        if (worldmapIsPlatformerArea(area)) {
            player.velY        = 0;
            player.grounded    = true;
            player.cameraX     = (player.pixelX / PLAY_AREA_W) * PLAY_AREA_W;
        } else {
            player.cameraX     = 0;
        }
        player.lastDrawnPixelX = PLAY_AREA_X + player.tileX * TILE_SIZE;
        player.lastDrawnPixelY = PLAY_AREA_Y + player.tileY * TILE_SIZE;
        uiSetArea(area);
        uiSetCameraX(player.cameraX);
        dropInitForArea(testMap, area);
        portalReset();
        if (area == AREA_ALTAR || area == AREA_TRADE || area == AREA_HOME) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                enemies[i].active = false;
                enemies[i].pendingDeathClear = false;
            }
        } else {
            enemyInit(testMap, player.tileX, player.tileY, area);
        }
        portalSpawn(testMap);
        gState      = STATE_EXPLORE;
        joySwNeedsRelease = true;
        tradeOpen = false;
        tradeDirty = false;
        tradeSelection = 0;
        tradeNotice = "";
        tradeNoticeError = false;
        screenDirty = true;
        projectilesInit();
        enemyDied = false;
        enemiesMoved = false;
        portalJustSpawned = false;
        boardClearSpawned = false;
        enteredAt   = 0;   // reset for the next time we enter worldmap
    }
    prevA = curA;
}
void gameUpdate() {
    updateInput();

    switch (gState) {
        case STATE_BOOT:     updateBoot();     break;
        case STATE_CG:       updateCG();       break;
        case STATE_CG2:      updateCG2();      break;
        case STATE_MENU:     updateMenu();     break;
        case STATE_EXPLORE:  updateExplore();  break;
        case STATE_WORLDMAP: updateWorldMap(); break;
    }
}

void gameRender() {
    // 状态切换时强制全量重绘
    if (gState != prevState) {
        screenDirty = true;
        prevState   = gState;
    }

    switch (gState) {
        case STATE_BOOT:
            if (screenDirty) {
                drawBootScreen();
                screenDirty = false;
            }
            break;

        case STATE_CG:
            if (screenDirty || cgNeedsRedraw()) {
                cgDraw();
                screenDirty = false;
            }
            break;

        case STATE_CG2:
            cg2Draw();
            screenDirty = false;
            break;

        case STATE_MENU:
            if (screenDirty) {
                drawMainMenu();
                screenDirty = false;
            }
            break;

        case STATE_EXPLORE: {
            static bool prevInventoryOpen = false;
            static bool prevTradeOpen = false;
            static int prevTradeSelection = 0;
            static int prevAltarSpeaker = 0;
            static bool prevOfferPrompt = false;
            static uint32_t altarDialogueCooldownUntil = 0;
            static int prevSupplySpeaker = 0;
            static bool prevMerchantPrompt = false;
            static uint32_t supplyDialogueCooldownUntil = 0;
            static bool miracleDialogueShown = false;
            bool altarOverlayInvalidated = false;
            bool supplyOverlayInvalidated = false;

            if (altarUnlockAnimating) {
                uint32_t elapsed = millis() - altarUnlockStarted;
                drawAltarWeaponRise(testMap, &player, elapsed);
                updateLaserRiseSound(elapsed);
                if (!miracleDialogueShown) {
                    drawSkullMiracleDialogue();
                    miracleDialogueShown = true;
                }
                prevPlayerX = player.tileX;
                prevPlayerY = player.tileY;
                prevInventoryOpen = false;
                prevTradeOpen = false;
                prevTradeSelection = tradeSelection;
                prevAltarSpeaker = 0;
                prevOfferPrompt = false;
                altarDialogueCooldownUntil = 0;
                prevSupplySpeaker = 0;
                prevMerchantPrompt = false;
                supplyDialogueCooldownUntil = 0;
                screenDirty = false;
                break;
            }
            miracleDialogueShown = false;

            if (screenDirty) {
                // Entering explore state fresh — full redraw
                drawExploreScreen(testMap, &player);
                latchEntityDrawPositions(&player);
                prevPlayerX = player.tileX;
                prevPlayerY = player.tileY;
                screenDirty  = false;
                statsDirty   = false;
                enemiesMoved = false;
                prevInventoryOpen = false;
                prevTradeOpen = false;
                prevTradeSelection = tradeSelection;
                prevAltarSpeaker = 0;
                prevOfferPrompt = false;
                altarDialogueCooldownUntil = 0;
                prevSupplySpeaker = 0;
                prevMerchantPrompt = false;
                supplyDialogueCooldownUntil = 0;
                altarOverlayInvalidated = true;
                supplyOverlayInvalidated = true;
            } else if (inventoryOpen && !prevInventoryOpen) {
                // Inventory just opened: draw overlay directly on top, no map redraw
                drawInventoryOverlay(&player);
                prevInventoryOpen = true;
            } else if (tradeOpen && !prevTradeOpen) {
                drawSupplyTradeOverlay(&player, tradeSelection, tradeNotice, tradeNoticeError);
                prevTradeOpen = true;
                prevTradeSelection = tradeSelection;
                tradeDirty = false;
            } else if (!inventoryOpen && prevInventoryOpen) {
                // Inventory just closed: redraw only the play area tiles (no fillScreen)
                drawPlayArea(testMap, &player);
                latchEntityDrawPositions(&player);
                prevPlayerX  = player.tileX;
                prevPlayerY  = player.tileY;
                enemiesMoved = false;
                prevInventoryOpen = false;
                prevTradeOpen = false;
                prevMerchantPrompt = false;
                altarOverlayInvalidated = true;
                supplyOverlayInvalidated = true;
            } else if (!tradeOpen && prevTradeOpen) {
                drawPlayArea(testMap, &player);
                latchEntityDrawPositions(&player);
                prevPlayerX  = player.tileX;
                prevPlayerY  = player.tileY;
                enemiesMoved = false;
                prevTradeOpen = false;
                prevMerchantPrompt = false;
                altarOverlayInvalidated = true;
                supplyOverlayInvalidated = true;
            } else if (tradeOpen && tradeDirty) {
                if ((!tradeNotice || !tradeNotice[0]) && prevTradeSelection != tradeSelection) {
                    drawSupplyTradeSelection(&player, prevTradeSelection, tradeSelection);
                } else {
                    drawSupplyTradeOverlay(&player, tradeSelection, tradeNotice, tradeNoticeError);
                }
                prevTradeSelection = tradeSelection;
                tradeDirty = false;
            } else if (enemyDied && !inventoryOpen) {
                // Enemy killed — clear the full 16x16 sprite rect of every
                // dead enemy (may span up to 4 tiles if they died mid-step),
                // then let drawSmoothEntitiesUpdate repaint remaining entities.
                drawPendingEnemyDeaths(testMap);
                if (boardClearSpawned) {
                    drawEnemyDeath(testMap, boardClearX, boardClearY);
                    boardClearSpawned = false;
                }
                if (portalJustSpawned) {
                    drawPortal();
                    portalJustSpawned = false;
                }
                enemyDied    = false;
                enemiesMoved = false;
                drawProjectilesUpdate(testMap, &player);
                drawSmoothEntitiesUpdate(testMap, &player);
            } else if (!inventoryOpen) {
                // Per-frame smooth redraw of player + all active enemies.
                // Returns immediately if nothing moved in pixel space.
                drawProjectilesUpdate(testMap, &player);
                drawSmoothEntitiesUpdate(testMap, &player);
                enemiesMoved = false;
            }
            prevPlayerX = player.tileX;
            prevPlayerY = player.tileY;

            // Refresh status bar if stats changed (enemy attack, etc.)
            if (statsDirty && !inventoryOpen && !tradeOpen) {
                drawStatusBar(&player);
                statsDirty = false;
            }

            if (!inventoryOpen && !tradeOpen && worldmapGetSelectedArea() == AREA_ALTAR) {
                int rawAltarSpeaker = 0;
                bool showOfferPrompt = altarNearOffering(player.tileX, player.tileY);
                bool hasOfferRelics = player.inv.alienRelics > 0;
                if (altarNearPriest(player.tileX, player.tileY)) rawAltarSpeaker = 1;
                else if (altarNearSkull(player.tileX, player.tileY)) rawAltarSpeaker = 2;

                int altarSpeaker = rawAltarSpeaker;
                if (prevAltarSpeaker == 0 && millis() < altarDialogueCooldownUntil) {
                    altarSpeaker = 0;
                }

                if (altarSpeaker == 0 && prevAltarSpeaker != 0) {
                    clearAltarDialogueRegion(testMap, &player);
                    altarDialogueCooldownUntil = millis() + 1200;
                    altarOverlayInvalidated = true;
                }
                if (altarSpeaker != 0 && prevAltarSpeaker != 0 && altarSpeaker != prevAltarSpeaker) {
                    clearAltarDialogueRegion(testMap, &player);
                    altarOverlayInvalidated = true;
                }
                if (showOfferPrompt != prevOfferPrompt) {
                    clearAltarDialogueRegion(testMap, &player);
                    altarOverlayInvalidated = true;
                }
                if (altarSpeaker == 1 && (altarOverlayInvalidated || prevAltarSpeaker != 1)) {
                    drawAltarDialogue();
                } else if (altarSpeaker == 2 && (altarOverlayInvalidated || prevAltarSpeaker != 2)) {
                    drawSkullDialogue();
                }
                if (showOfferPrompt && (altarOverlayInvalidated || !prevOfferPrompt)) {
                    drawOfferRelicsPrompt(hasOfferRelics);
                }
                prevAltarSpeaker = altarSpeaker;
                prevOfferPrompt = showOfferPrompt;
                prevSupplySpeaker = 0;
            } else {
                prevAltarSpeaker = 0;
                prevOfferPrompt = false;
                if (!inventoryOpen && !tradeOpen && worldmapGetSelectedArea() == AREA_TRADE) {
                    int rawSupplySpeaker = 0;
                    bool showMerchantPrompt = supplyNearMerchant(player.tileX, player.tileY);
                    if (supplyNearWarrior(player.tileX, player.tileY)) rawSupplySpeaker = 1;
                    else if (supplyNearCivilian(player.tileX, player.tileY)) rawSupplySpeaker = 2;
                    else if (supplyNearWizard(player.tileX, player.tileY)) rawSupplySpeaker = 3;

                    int supplySpeaker = rawSupplySpeaker;
                    if (prevSupplySpeaker == 0 && millis() < supplyDialogueCooldownUntil) {
                        supplySpeaker = 0;
                    }

                    if (supplySpeaker == 0 && prevSupplySpeaker != 0) {
                        clearSupplyDialogueRegion(testMap, &player);
                        supplyDialogueCooldownUntil = millis() + 1200;
                        supplyOverlayInvalidated = true;
                    }
                    if (supplySpeaker != 0 && prevSupplySpeaker != 0 && supplySpeaker != prevSupplySpeaker) {
                        clearSupplyDialogueRegion(testMap, &player);
                        supplyOverlayInvalidated = true;
                    }
                    if (showMerchantPrompt != prevMerchantPrompt) {
                        clearSupplyDialogueRegion(testMap, &player);
                        supplyOverlayInvalidated = true;
                    }

                    if (supplySpeaker == 1 && (supplyOverlayInvalidated || prevSupplySpeaker != 1)) {
                        drawSupplyWarriorDialogue();
                    } else if (supplySpeaker == 2 && (supplyOverlayInvalidated || prevSupplySpeaker != 2)) {
                        drawSupplyCivilianDialogue();
                    } else if (supplySpeaker == 3 && (supplyOverlayInvalidated || prevSupplySpeaker != 3)) {
                        drawSupplyWizardDialogue();
                    }
                    if (showMerchantPrompt && (supplyOverlayInvalidated || !prevMerchantPrompt)) {
                        drawSupplyTradePrompt();
                    }
                    prevSupplySpeaker = supplySpeaker;
                    prevMerchantPrompt = showMerchantPrompt;
                } else {
                    prevSupplySpeaker = 0;
                    prevMerchantPrompt = false;
                }
            }
            break;
        }  // end STATE_EXPLORE

        case STATE_WORLDMAP:
            if (screenDirty) {
                worldmapDraw();
                screenDirty = false;
            }
            break;
    }
}
