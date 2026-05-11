#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include "config.h"
#include "game.h"
#include "weapon.h"
#include "worldmap.h"

// Set the active area ID so drawTile uses the matching visual theme
void uiSetArea(int areaId);
void uiSetCameraX(int cx);

void drawBootScreen();
void drawMainMenu();
void drawExploreScreen(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player);
void drawExploreUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player, int oldX, int oldY);
void drawPlayArea(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player);
void drawInventoryOverlay(Player* player);

// Refresh just the status bar (call when HP/hunger/armor changes)
void drawStatusBar(Player* p);

// Redraw the bottom action bar (weapon icon, backpack, map name)
void drawBottomBar(int weaponType);

// Only update the food quantity number inside the inventory overlay (no panel redraw)
void drawInventoryFoodCount(int qty);
void drawInventoryArmorCount(int qty);

// Only redraw the weapon slot (slot 1) — no flicker on weapon switch
void drawWeaponSlot(int weaponType);

// Only redraw the tile where an enemy just died (restore map + draw drop)
void drawEnemyDeath(uint8_t mapData[MAP_ROWS][MAP_COLS], int tileX, int tileY);

// Iterate enemies[] and, for each one flagged pendingDeathClear, redraw every
// tile overlapping its last-drawn 16x16 pixel rect (up to 4 tiles). Needed
// because mid-step deaths leave a sprite straddling two tiles that a single
// tile redraw would not fully erase.
void drawPendingEnemyDeaths(uint8_t mapData[MAP_ROWS][MAP_COLS]);

// Incremental enemy redraw: erase old tiles, draw new positions (no full map refresh)
void drawEnemyUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS]);

// Smooth (pixel-level) redraw of player + all active enemies.
// Each entity stores its previous frame's draw pixel position in lastDrawnPixelX/Y.
// Call this every frame during STATE_EXPLORE (when no overlay is active).
void drawSmoothEntitiesUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player);
void drawProjectilesUpdate(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player);

// Draw a brief bullet-trail flash from player tile to target tile, then erase.
// player is passed so smooth-interpolated sprites can be restored after the flash.
void drawBulletFlash(uint8_t mapData[MAP_ROWS][MAP_COLS],
                     int playerX, int playerY,
                     int targetX, int targetY,
                     int weaponType,
                     Player* player);

// Draw a red laser beam fired by a laser alien, then erase.
// player is needed so the player sprite can be redrawn if the beam passes through their tile.
void drawLaserFlash(uint8_t mapData[MAP_ROWS][MAP_COLS],
                    int startX, int startY,
                    int endX,   int endY,
                    Player* player);

// Draw a massive cyan laser cannon beam (player's weapon)
void drawLaserCannonFlash(uint8_t mapData[MAP_ROWS][MAP_COLS],
                          int startTileX, int startTileY, int dir, bool isPlatformer,
                          Player* player, int recoilX = 0, int recoilY = 0);

// Draw the black-hole portal sprite on a single tile (no surrounding redraw)
void drawPortalTile(int tileX, int tileY);

// If portal is active, draw it — safe to call after any incremental update
void drawPortal();

// Update only slot 3 of the bottom bar (map name / portal prompt)
// nearPortal=true  → shows "[D] ENTER" prompt
// nearPortal=false → shows normal map name
void drawPortalSlot(bool nearPortal);
void drawAltarDialogue();
void drawSkullDialogue();
void drawSkullMiracleDialogue();
void clearAltarDialogueRegion(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player);
void drawSupplyWarriorDialogue();
void drawSupplyCivilianDialogue();
void drawSupplyWizardDialogue();
void clearSupplyDialogueRegion(uint8_t mapData[MAP_ROWS][MAP_COLS], Player* player);
void drawSupplyTradePrompt();
void drawSupplyTradeOverlay(Player* player, int selection, const char* notice, bool noticeError);
void drawSupplyTradeSelection(Player* player, int oldSelection, int newSelection);
void drawOfferRelicsPrompt(bool hasRelics);
void drawAltarWeaponRise(uint8_t mapData[MAP_ROWS][MAP_COLS],
                         Player* player,
                         uint32_t elapsedMs);
#endif
