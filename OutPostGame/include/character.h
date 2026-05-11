#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdint.h>

#define CHARACTER_SIZE 16

// Direction constants (used by characterSetDir to drive walk animation)
#define CHAR_DIR_UP    0
#define CHAR_DIR_DOWN  1
#define CHAR_DIR_LEFT  2
#define CHAR_DIR_RIGHT 3

void characterInit();
extern bool g_playerCorrupted;
// `scale` selects the on-screen sprite size: 2 → 16x16 px (1 px/bit, safe
// zones), 3 → 24x24 px (1.5 px/bit via offset table, combat zones). Sprite is
// centred on its 16x16 tile anchor (`px`,`py`) — at scale 3 it extends 4 px
// past each edge.
void characterDraw(int px, int py, int weaponType, int scale = 2, uint32_t lastFire = 0);
// Opaque variant: every pixel inside the sprite rect is written exactly once
// (sprite color for set bits, bg for unset). Use for smooth-moving redraw so
// stale pixels from the previous frame can't leak through unset bits.
void characterDrawOpaque(int px, int py, int weaponType, uint16_t bg, int scale = 2, uint32_t lastFire = 0);
void characterSetDir(int dir);                      // advance walk frame (dir kept for future use)
int  characterGetDir();
void characterDrawFarmer(int px, int py);
void characterDrawMerchant(int px, int py);
void characterDrawPriest(int px, int py);
void characterDrawSkull(int px, int py);
void characterDrawRedVisorScout(int px, int py);
void characterDrawCivilian(int px, int py);
void characterDrawWizard(int px, int py);

#endif
