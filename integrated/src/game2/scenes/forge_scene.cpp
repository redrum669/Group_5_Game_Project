#include "game2/scenes/forge_scene.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "game2/app/game_app.h"
#include "game2/config/game_config.h"
#include "game2/config/reference/reference_style.h"
#include "game2/core/scene_id.h"
#include "game2/drivers/lcd_driver.h"
#include "game2/drivers/platform_backend.h"
#include "game2/ui/character_renderer.h"

// ---------------------------------------------------------------------------
// Colors
// ---------------------------------------------------------------------------
#define FORGE_PANEL_COLOR     0x2104
#define FORGE_WALL_COLOR      0x6241
#define FORGE_WALL_DARK       0x4208
#define FORGE_FLOOR_LT        0x8A62
#define FORGE_FLOOR_DK        0x7BEF
#define FORGE_FLOOR_PATTERN   0x6A45
#define FIREPLACE_BRICK       0xB596
#define FIREPLACE_DARK        0x8A62
#define FIRE_FLAME1           0xF800
#define FIRE_FLAME2           0xFD20
#define FIRE_FLAME3           0xFFE0
#define ANVIL_METAL           0x9CD3
#define ANVIL_DARK            0x7BEF
#define ANVIL_TOP             0xC618
#define ANVIL_BASE            0x528A
#define SELECT_HIGHLIGHT      0x04BF
#define CRAFT_BG              0x18C3

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
#define PLAYER_W GAME_REF_CHARACTER_RENDER_SIZE
#define PLAYER_H GAME_REF_CHARACTER_RENDER_SIZE

#define ANVIL_X  148
#define ANVIL_Y  108
#define ANVIL_W  44
#define ANVIL_H  32
#define ANVIL_HIT_X (ANVIL_X - 16)
#define ANVIL_HIT_Y (ANVIL_Y - 16)
#define ANVIL_HIT_W (ANVIL_W + 32)
#define ANVIL_HIT_H (ANVIL_H + 32)

#define FIREPLACE_X 24
#define FIREPLACE_Y 80
#define FIREPLACE_W 48
#define FIREPLACE_H 44

#define CRAFT_LIST_X 30
#define CRAFT_LIST_Y 56
#define CRAFT_ITEM_H 26
#define CRAFT_ITEM_W 180

#define RACK_Y      (GAME_STATUS_BAR_HEIGHT + 10)
#define RACK_H      30
#define RACK_SLOT_W 52
#define RACK_X0     6
#define RACK_GAP    4

#define FORGE_TIME_MS 3000u

// ---------------------------------------------------------------------------
// Weapon catalog (mirrors reference/weapon.h)
// ---------------------------------------------------------------------------
typedef enum {
  WEAPON_SMG = 0,
  WEAPON_MG,
  WEAPON_LASER,
  WEAPON_LASER_GUN,
  WEAPON_COUNT
} forge_weapon_id_t;

static const char* s_weapon_names[WEAPON_COUNT] = {
  "SMG",
  "MACHINE GUN",
  "LASER SWORD",
  "LASER GUN"
};

static const char* s_rack_labels[WEAPON_COUNT] = {
  "SMG",
  "MG",
  "BLADE",
  "RIFLE"
};

#define WEAPON_UNLOCK_BIT(t) (1u << (t))

// ---------------------------------------------------------------------------
// Mode
// ---------------------------------------------------------------------------
typedef enum {
  FORGE_MODE_EXPLORE = 0,
  FORGE_MODE_CRAFT,
  FORGE_MODE_CRAFTING,
  FORGE_MODE_CRAFT_DONE
} forge_mode_t;

static bool s_forge_force_full_redraw = false;

// ---------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------
static void draw_stone_floor(void) {
  int16_t y;
  for (y = GAME_STATUS_BAR_HEIGHT; y < 200; y += 10) {
    uint16_t c = (((y / 10) & 1) == 0) ? FORGE_FLOOR_LT : FORGE_FLOOR_DK;
    game_lcd_fill_rect(0, y, GAME_SCREEN_WIDTH, 10, c);
  }
  // Random stone speckles
  for (y = GAME_STATUS_BAR_HEIGHT + 3; y < 200; y += 12) {
    int16_t x;
    int16_t off = ((y / 12) & 1) ? 5 : 0;
    for (x = 6 + off; x < GAME_SCREEN_WIDTH - 6; x += 18) {
      game_lcd_fill_rect(x, y, 3, 2, FORGE_FLOOR_PATTERN);
    }
  }
}

static void draw_walls(void) {
  game_lcd_fill_rect(0, GAME_STATUS_BAR_HEIGHT, GAME_SCREEN_WIDTH, 6, FORGE_WALL_COLOR);
  game_lcd_fill_rect(0, 194, GAME_SCREEN_WIDTH, 6, FORGE_WALL_DARK);
  game_lcd_fill_rect(0, GAME_STATUS_BAR_HEIGHT, 6, 200 - GAME_STATUS_BAR_HEIGHT, FORGE_WALL_COLOR);
  game_lcd_fill_rect(GAME_SCREEN_WIDTH - 6, GAME_STATUS_BAR_HEIGHT, 6, 200 - GAME_STATUS_BAR_HEIGHT, FORGE_WALL_COLOR);
}

static void draw_fireplace(void) {
  int16_t x = FIREPLACE_X;
  int16_t y = FIREPLACE_Y;

  // Brick body
  game_lcd_fill_rect(x, y, FIREPLACE_W, FIREPLACE_H, FIREPLACE_BRICK);
  game_lcd_draw_rect(x, y, FIREPLACE_W, FIREPLACE_H, FIREPLACE_DARK);

  // Fire pit interior
  game_lcd_fill_rect(x + 6, y + 8, FIREPLACE_W - 12, FIREPLACE_H - 14, 0x18A3);

  // Flames (static layers)
  game_lcd_fill_rect(x + 14, y + 18, 6, 10, FIRE_FLAME1);
  game_lcd_fill_rect(x + 20, y + 14, 6, 14, FIRE_FLAME2);
  game_lcd_fill_rect(x + 26, y + 20, 5, 8, FIRE_FLAME1);
  game_lcd_fill_rect(x + 18, y + 22, 4, 4, FIRE_FLAME3);
  game_lcd_fill_rect(x + 24, y + 16, 4, 6, FIRE_FLAME3);

  // Chimney
  game_lcd_fill_rect(x + 14, y - 10, 18, 12, FIREPLACE_BRICK);
  game_lcd_fill_rect(x + 18, y - 14, 10, 6, 0x9CD3);
}

static void draw_weapon_icon(int16_t cx, int16_t cy, int weapon_type, bool unlocked) {
  uint16_t color = unlocked ? GAME_REF_COLOR_CYAN : 0x7BEF;
  uint16_t dark  = unlocked ? 0x04BF : 0x4208;

  switch (weapon_type) {
    case WEAPON_SMG: {
      // Compact body + short barrel
      game_lcd_fill_rect(cx - 8, cy - 2, 14, 6, color);
      game_lcd_fill_rect(cx + 4, cy - 1, 8, 3, color);
      game_lcd_fill_rect(cx - 6, cy + 4, 4, 2, dark);
      break;
    }
    case WEAPON_MG: {
      // Long body + long barrel + stock
      game_lcd_fill_rect(cx - 10, cy - 3, 18, 7, color);
      game_lcd_fill_rect(cx + 6, cy - 1, 10, 3, color);
      game_lcd_fill_rect(cx - 14, cy - 2, 5, 6, dark);
      break;
    }
    case WEAPON_LASER: {
      // Sword blade + hilt
      game_lcd_fill_rect(cx - 1, cy - 10, 3, 18, color);  // blade
      game_lcd_fill_rect(cx - 4, cy + 6, 9, 3, dark);      // crossguard
      game_lcd_fill_rect(cx - 2, cy + 8, 5, 4, 0x8A62);   // handle
      break;
    }
    case WEAPON_LASER_GUN: {
      // Large body + scope + long barrel
      game_lcd_fill_rect(cx - 10, cy - 4, 16, 9, color);
      game_lcd_fill_rect(cx + 4, cy - 2, 10, 4, color);
      game_lcd_fill_rect(cx - 6, cy - 7, 6, 4, 0x9CD3);   // scope
      break;
    }
    default:
      break;
  }
}

static void draw_weapon_rack(uint8_t unlocked_mask) {
  int16_t rack_left = RACK_X0;
  int16_t rack_right = (int16_t)(RACK_X0 + 4 * RACK_SLOT_W + 3 * RACK_GAP);
  int16_t y = RACK_Y;

  // Wooden backboard
  game_lcd_fill_rect(rack_left, y, rack_right - rack_left, RACK_H, 0x6241);
  game_lcd_draw_rect(rack_left, y, rack_right - rack_left, RACK_H, 0x8A62);

  // Horizontal shelf line
  game_lcd_fill_rect(rack_left + 2, y + RACK_H - 4, rack_right - rack_left - 4, 3, 0x8A62);

  int i;
  for (i = 0; i < WEAPON_COUNT; i++) {
    int16_t slot_x = (int16_t)(RACK_X0 + i * (RACK_SLOT_W + RACK_GAP));
    int16_t cx = (int16_t)(slot_x + RACK_SLOT_W / 2);
    int16_t cy = (int16_t)(y + RACK_H / 2 + 2);
    bool unlocked = (unlocked_mask & WEAPON_UNLOCK_BIT(i)) != 0;

    // Hook / peg
    game_lcd_fill_rect(cx - 2, y + 4, 5, 3, 0x8A62);

    // Weapon icon
    draw_weapon_icon(cx, cy, i, unlocked);

    // Label below icon (shortened to fit slot width)
    game_lcd_draw_text(slot_x + 4, (int16_t)(y + RACK_H + 2),
                       s_rack_labels[i],
                       unlocked ? GAME_REF_COLOR_CYAN : 0x7BEF,
                       FORGE_FLOOR_LT, 1);
  }
}

static void draw_anvil(void) {
  int16_t x = ANVIL_X;
  int16_t y = ANVIL_Y;

  // Base stump
  game_lcd_fill_rect(x + 8, y + 14, 28, 18, ANVIL_BASE);
  game_lcd_draw_rect(x + 8, y + 14, 28, 18, 0x4208);

  // Anvil body
  game_lcd_fill_rect(x, y + 6, ANVIL_W, 10, ANVIL_METAL);
  game_lcd_draw_rect(x, y + 6, ANVIL_W, 10, ANVIL_DARK);

  // Top horn
  game_lcd_fill_rect(x - 6, y + 4, 10, 8, ANVIL_METAL);
  game_lcd_fill_rect(x + 34, y + 4, 10, 8, ANVIL_METAL);

  // Top flat face
  game_lcd_fill_rect(x + 2, y + 4, ANVIL_W - 4, 4, ANVIL_TOP);
}

static void draw_forge_hud(const char* action, const char* status) {
  game_lcd_fill_rect(0, 200, GAME_SCREEN_WIDTH, 40, FORGE_PANEL_COLOR);
  game_lcd_draw_rect(0, 200, GAME_SCREEN_WIDTH, 40, GAME_REF_COLOR_WHITE);
  game_lcd_draw_text(8, 206, action, GAME_REF_COLOR_YELLOW, FORGE_PANEL_COLOR, 1);
  game_lcd_draw_text(8, 218, status, GAME_REF_COLOR_WHITE, FORGE_PANEL_COLOR, 1);
}

static void redraw_floor_patch(int16_t x, int16_t y, int16_t w, int16_t h) {
  int16_t x0 = x;
  int16_t y0 = y;
  int16_t x1 = x + w;
  int16_t y1 = y + h;
  if (x0 < GAME_PLAY_AREA_X) x0 = GAME_PLAY_AREA_X;
  if (y0 < GAME_STATUS_BAR_HEIGHT) y0 = GAME_STATUS_BAR_HEIGHT;
  if (x1 > GAME_PLAY_AREA_X + GAME_PLAY_AREA_WIDTH)  x1 = GAME_PLAY_AREA_X + GAME_PLAY_AREA_WIDTH;
  if (y1 > GAME_PLAY_AREA_Y + GAME_PLAY_AREA_HEIGHT) y1 = GAME_PLAY_AREA_Y + GAME_PLAY_AREA_HEIGHT;
  if (x1 <= x0 || y1 <= y0) return;

  int16_t py;
  // Pass 1: stripes (must match draw_stone_floor exactly)
  for (py = y0; py < y1; py++) {
    int16_t stripe_idx = (py - GAME_STATUS_BAR_HEIGHT) / 10;
    uint16_t base = ((stripe_idx & 1) == 0) ? FORGE_FLOOR_LT : FORGE_FLOOR_DK;
    game_lcd_fill_rect(x0, py, x1 - x0, 1, base);
  }
  // Pass 2: deterministic speckles (must match draw_stone_floor exactly)
  int16_t first_speckle_y = GAME_STATUS_BAR_HEIGHT + 3;
  if (first_speckle_y < y0) {
    int16_t n = (y0 - first_speckle_y + 11) / 12;
    first_speckle_y += n * 12;
  }
  for (py = first_speckle_y; py < y1; py += 12) {
    int16_t off = ((py / 12) & 1) ? 5 : 0;
    int16_t sx;
    for (sx = 6 + off; sx < GAME_SCREEN_WIDTH - 6; sx += 18) {
      if (sx + 3 > x0 && sx < x1) {
        int16_t draw_x = sx;
        int16_t draw_w = 3;
        if (draw_x < x0) { draw_w -= (x0 - draw_x); draw_x = x0; }
        if (draw_x + draw_w > x1) draw_w = x1 - draw_x;
        int16_t draw_y = py;
        int16_t draw_h = 2;
        if (draw_y < y0) { draw_h -= (y0 - draw_y); draw_y = y0; }
        if (draw_y + draw_h > y1) draw_h = y1 - draw_y;
        if (draw_w > 0 && draw_h > 0) {
          game_lcd_fill_rect(draw_x, draw_y, draw_w, draw_h, FORGE_FLOOR_PATTERN);
        }
      }
    }
  }
}

static bool rect_intersects(int16_t x, int16_t y, int16_t w, int16_t h,
                            int16_t rx, int16_t ry, int16_t rw, int16_t rh) {
  return !(x + w <= rx || rx + rw <= x || y + h <= ry || ry + rh <= y);
}

static void redraw_explore_patch(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t unlocked_mask) {
  // Restore correct stone-floor texture (stripes + spots)
  redraw_floor_patch(x, y, w, h);

  // Re-draw any static object whose bounding box intersects the patch
  if (rect_intersects(x, y, w, h, FIREPLACE_X, FIREPLACE_Y, FIREPLACE_W, FIREPLACE_H)) {
    draw_fireplace();
  }
  if (rect_intersects(x, y, w, h, ANVIL_X - 6, ANVIL_Y, ANVIL_W + 12, ANVIL_H + 2)) {
    draw_anvil();
  }
  {
    int16_t rack_right = (int16_t)(RACK_X0 + 4 * RACK_SLOT_W + 3 * RACK_GAP);
    if (rect_intersects(x, y, w, h, RACK_X0, RACK_Y, rack_right - RACK_X0, RACK_H + 10)) {
      draw_weapon_rack(unlocked_mask);
    }
  }
  // Walls
  if (rect_intersects(x, y, w, h, 0, GAME_STATUS_BAR_HEIGHT, GAME_SCREEN_WIDTH, 6)) {
    game_lcd_fill_rect(0, GAME_STATUS_BAR_HEIGHT, GAME_SCREEN_WIDTH, 6, FORGE_WALL_COLOR);
  }
  if (rect_intersects(x, y, w, h, 0, 194, GAME_SCREEN_WIDTH, 6)) {
    game_lcd_fill_rect(0, 194, GAME_SCREEN_WIDTH, 6, FORGE_WALL_DARK);
  }
  if (rect_intersects(x, y, w, h, 0, GAME_STATUS_BAR_HEIGHT, 6, 200 - GAME_STATUS_BAR_HEIGHT)) {
    game_lcd_fill_rect(0, GAME_STATUS_BAR_HEIGHT, 6, 200 - GAME_STATUS_BAR_HEIGHT, FORGE_WALL_COLOR);
  }
  if (rect_intersects(x, y, w, h, GAME_SCREEN_WIDTH - 6, GAME_STATUS_BAR_HEIGHT, 6, 200 - GAME_STATUS_BAR_HEIGHT)) {
    game_lcd_fill_rect(GAME_SCREEN_WIDTH - 6, GAME_STATUS_BAR_HEIGHT, 6, 200 - GAME_STATUS_BAR_HEIGHT, FORGE_WALL_COLOR);
  }
}

static void draw_explore_scene(int16_t player_x, int16_t player_y, bool near_anvil, uint8_t unlocked_mask) {
  draw_stone_floor();
  draw_walls();
  draw_fireplace();
  draw_anvil();
  draw_weapon_rack(unlocked_mask);

  // Player shadow
  game_lcd_fill_rect(player_x + 4, player_y + PLAYER_H - 2, 8, 3, 0x0841);
  game_character_renderer_draw(player_x, player_y);

  // Top bar
  game_lcd_draw_rect(0, 0, GAME_SCREEN_WIDTH, GAME_STATUS_BAR_HEIGHT, GAME_REF_COLOR_WHITE);
  game_lcd_draw_text(8, 7, "FORGE", GAME_REF_COLOR_CYAN, GAME_REF_COLOR_BLACK, 1);

  // Play border
  game_lcd_draw_rect(0,
                     GAME_STATUS_BAR_HEIGHT,
                     GAME_SCREEN_WIDTH,
                     GAME_PLAY_AREA_HEIGHT,
                     GAME_REF_COLOR_WHITE);

  if (near_anvil) {
    draw_forge_hud("A:FORGE WEAPON", "B:RETURN TO WORLD");
  } else {
    draw_forge_hud("MOVE WITH JOYSTICK", "B:RETURN TO WORLD");
  }
}

static void draw_craft_item(uint8_t idx, bool selected, uint8_t unlocked_mask) {
  int16_t iy = (int16_t)(CRAFT_LIST_Y + idx * CRAFT_ITEM_H);
  uint16_t bg = selected ? SELECT_HIGHLIGHT : CRAFT_BG;
  uint16_t border = selected ? GAME_REF_COLOR_WHITE : 0x7BEF;

  game_lcd_fill_rect(CRAFT_LIST_X, iy, CRAFT_ITEM_W, CRAFT_ITEM_H - 2, bg);
  game_lcd_draw_rect(CRAFT_LIST_X, iy, CRAFT_ITEM_W, CRAFT_ITEM_H - 2, border);

  game_lcd_draw_text(CRAFT_LIST_X + 6, (int16_t)(iy + 6),
                     s_weapon_names[idx],
                     GAME_REF_COLOR_WHITE, bg, 1);

  bool unlocked = (unlocked_mask & WEAPON_UNLOCK_BIT(idx)) != 0;
  game_lcd_draw_text(CRAFT_LIST_X + 140, (int16_t)(iy + 6),
                     unlocked ? "OWNED" : "FORGE",
                     unlocked ? GAME_REF_COLOR_CYAN : GAME_REF_COLOR_YELLOW,
                     bg, 1);
}

static void draw_craft_menu(uint8_t selected, uint8_t unlocked_mask) {
  // Dark overlay
  game_lcd_fill_rect(16, 42, 208, 146, CRAFT_BG);
  game_lcd_draw_rect(16, 42, 208, 146, GAME_REF_COLOR_WHITE);

  game_lcd_draw_text(24, 48, "SELECT WEAPON TO FORGE",
                     GAME_REF_COLOR_YELLOW, CRAFT_BG, 1);

  int i;
  for (i = 0; i < WEAPON_COUNT; i++) {
    draw_craft_item((uint8_t)i, (i == (int)selected), unlocked_mask);
  }

  // Cost line
  game_lcd_draw_text(24, 162, "COST: FOOD 0  METAL 0  RELIC 0",
                     GAME_REF_COLOR_WHITE, CRAFT_BG, 1);

  game_lcd_draw_text(24, 176, "UP/DOWN:SELECT  A:FORGE  B:BACK",
                     GAME_REF_COLOR_YELLOW, CRAFT_BG, 1);
}

static void draw_crafting_anim(uint32_t elapsed_ms, uint8_t unlocked_mask) {
  static int16_t prev_s1x = -1, prev_s1y = -1;
  static int16_t prev_s2x = -1, prev_s2y = -1;
  static int16_t prev_s3x = -1, prev_s3y = -1;
  static int16_t prev_flash_x = -1, prev_flash_y = -1;

  // Erase old sparks / flash
  if (prev_s1x >= 0)    redraw_explore_patch(prev_s1x, prev_s1y, 4, 4, unlocked_mask);
  if (prev_s2x >= 0)    redraw_explore_patch(prev_s2x, prev_s2y, 3, 3, unlocked_mask);
  if (prev_s3x >= 0)    redraw_explore_patch(prev_s3x, prev_s3y, 2, 2, unlocked_mask);
  if (prev_flash_x >= 0) redraw_explore_patch(prev_flash_x, prev_flash_y, 14, 6, unlocked_mask);

  // Anvil flash on hammer hit (every ~350ms)
  if ((elapsed_ms % 350) < 60) {
    prev_flash_x = ANVIL_X + 4;
    prev_flash_y = ANVIL_Y + 2;
    game_lcd_fill_rect(ANVIL_X + 4, ANVIL_Y + 2, 14, 4, FIRE_FLAME3);
    game_lcd_fill_rect(ANVIL_X + 8, ANVIL_Y, 6, 2, GAME_REF_COLOR_WHITE);
  } else {
    prev_flash_x = -1;
  }

  // Multi-particle spark shower
  uint32_t seed = elapsed_ms * 1103515245u + 12345u;
  int16_t s1x = (int16_t)(ANVIL_X + 6 + (seed % 22));
  int16_t s1y = (int16_t)(ANVIL_Y - 8 + ((seed >> 3) % 12));
  int16_t s2x = (int16_t)(ANVIL_X + 10 + ((seed >> 5) % 18));
  int16_t s2y = (int16_t)(ANVIL_Y - 5 + ((seed >> 7) % 10));
  int16_t s3x = (int16_t)(ANVIL_X + 4 + ((seed >> 9) % 20));
  int16_t s3y = (int16_t)(ANVIL_Y - 3 + ((seed >> 11) % 8));

  game_lcd_fill_rect(s1x, s1y, 4, 4, FIRE_FLAME3);
  game_lcd_fill_rect(s2x, s2y, 3, 3, FIRE_FLAME2);
  game_lcd_fill_rect(s3x, s3y, 2, 2, FIRE_FLAME1);

  prev_s1x = s1x; prev_s1y = s1y;
  prev_s2x = s2x; prev_s2y = s2y;
  prev_s3x = s3x; prev_s3y = s3y;

  // Progress bar
  int16_t bw = (int16_t)(elapsed_ms * 180 / FORGE_TIME_MS);
  if (bw > 180) bw = 180;

  // Bar background (use CRAFT_BG for consistency with menu overlay)
  game_lcd_fill_rect(30, 170, 180, 10, CRAFT_BG);
  // Bar fill (warm orange)
  game_lcd_fill_rect(30, 170, bw, 10, FIRE_FLAME2);
  // Highlight tip
  if (bw > 4) {
    game_lcd_fill_rect((int16_t)(30 + bw - 4), 170, 4, 10, FIRE_FLAME3);
  }
  // Bar border
  game_lcd_draw_rect(30, 170, 180, 10, GAME_REF_COLOR_WHITE);

  // Text below bar to avoid overlap with progress fill
  game_lcd_draw_text(80, 182, "FORGING...", GAME_REF_COLOR_WHITE, CRAFT_BG, 1);
}

static void draw_craft_done(const char* weapon_name) {
  game_lcd_fill_rect(40, 80, 160, 60, CRAFT_BG);
  game_lcd_draw_rect(40, 80, 160, 60, GAME_REF_COLOR_WHITE);
  game_lcd_draw_text(50, 90, "FORGE SUCCESS!", GAME_REF_COLOR_YELLOW, CRAFT_BG, 1);
  game_lcd_draw_text(50, 108, weapon_name, GAME_REF_COLOR_CYAN, CRAFT_BG, 1);
  game_lcd_draw_text(50, 126, "A:CONTINUE", GAME_REF_COLOR_WHITE, CRAFT_BG, 1);
}

// ---------------------------------------------------------------------------
// Logic
// ---------------------------------------------------------------------------
static bool is_near_anvil(int16_t px, int16_t py) {
  int16_t cx = (int16_t)(px + PLAYER_W / 2);
  int16_t cy = (int16_t)(py + PLAYER_H / 2);
  int16_t ax = ANVIL_X + ANVIL_W / 2;
  int16_t ay = ANVIL_Y + ANVIL_H / 2;
  int16_t dx = cx - ax;
  int16_t dy = cy - ay;
  return (dx * dx + dy * dy) <= (50 * 50);
}

static bool is_blocked_explore(int16_t px, int16_t py) {
  if (px < GAME_PLAY_AREA_X + 4) return true;
  if (py < GAME_STATUS_BAR_HEIGHT + 8) return true;
  if (px > GAME_SCREEN_WIDTH - PLAYER_W - 4) return true;
  if (py > 192 - PLAYER_H) return true;
  // Fireplace collision
  if (px + PLAYER_W > FIREPLACE_X + 4 && px < FIREPLACE_X + FIREPLACE_W - 4 &&
      py + PLAYER_H > FIREPLACE_Y + 4 && py < FIREPLACE_Y + FIREPLACE_H - 4) return true;
  // Anvil collision
  if (px + PLAYER_W > ANVIL_X - 2 && px < ANVIL_X + ANVIL_W + 2 &&
      py + PLAYER_H > ANVIL_Y + 4 && py < ANVIL_Y + ANVIL_H + 2) return true;
  return false;
}

static void play_hammer_sound(uint32_t* last_hammer_ms) {
  uint32_t now = game_platform_millis();
  if (now - *last_hammer_ms >= 300u) {
    game_buzzer_tone(800, 80);
    *last_hammer_ms = now;
  }
}

static void play_forge_success_melody(uint8_t* idx, uint32_t* note_ms) {
  static const uint16_t freqs[] = {587, 740, 880, 1175, 0};
  static const uint16_t durs[]  = {150, 150, 150, 450, 0};
  uint32_t now = game_platform_millis();

  if (*idx == 0 && *note_ms == 0) {
    *note_ms = now;
    game_buzzer_tone(freqs[0], durs[0]);
  } else if (freqs[*idx] != 0) {
    if (now - *note_ms >= durs[*idx]) {
      (*idx)++;
      *note_ms = now;
      if (freqs[*idx] != 0) {
        game_buzzer_tone(freqs[*idx], durs[*idx]);
      } else {
        game_buzzer_stop();
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Main update
// ---------------------------------------------------------------------------
void game_forge_scene_update(void) {
  static bool initialized = false;
  static int16_t player_x = 80;
  static int16_t player_y = 140;
  static int16_t prev_player_x = 80;
  static int16_t prev_player_y = 140;
  static uint32_t last_move_ms = 0;
  static forge_mode_t mode = FORGE_MODE_EXPLORE;
  static uint8_t craft_selected = 0;
  static uint32_t craft_start_ms = 0;
  static uint32_t hammer_last_ms = 0;
  static uint8_t melody_idx = 0;
  static uint32_t melody_note_ms = 0;
  static bool prev_up = false;
  static bool prev_down = false;

  bool need_full = false;
  bool moved = false;

  if (s_forge_force_full_redraw) {
    initialized = false;
    s_forge_force_full_redraw = false;
  }

  if (!initialized) {
    game_character_renderer_init();
    player_x = 80;
    player_y = 140;
    prev_player_x = player_x;
    prev_player_y = player_y;
    mode = FORGE_MODE_EXPLORE;
    craft_selected = 0;
    melody_idx = 0;
    melody_note_ms = 0;
    game_buzzer_stop();
    initialized = true;
    need_full = true;
  }

  game_input_update();

  // B button: back / cancel
  if (game_input_button_b_edge()) {
    if (mode == FORGE_MODE_CRAFT || mode == FORGE_MODE_CRAFT_DONE) {
      mode = FORGE_MODE_EXPLORE;
      game_buzzer_stop();
      need_full = true;
    } else {
      initialized = false;
      game_buzzer_stop();
      game_app_request_scene(GAME_SCENE_WORLD);
      return;
    }
  }

  game_shared_resources_t* shared = game_app_shared_resources();
  uint8_t unlocked_mask = (shared != 0) ? shared->weapon_unlocked_mask : 0;

  // Mode state machine
  switch (mode) {
    case FORGE_MODE_EXPLORE: {
      if (game_platform_millis() - last_move_ms >= 16u) {
        last_move_ms = game_platform_millis();
        int16_t next_x = player_x;
        int16_t next_y = player_y;

        if (game_input_up()) {
          next_y -= 2;
          game_character_renderer_set_dir(GAME_CHARACTER_DIR_UP);
        }
        if (game_input_down()) {
          next_y += 2;
          game_character_renderer_set_dir(GAME_CHARACTER_DIR_DOWN);
        }
        if (game_input_left()) {
          next_x -= 2;
          game_character_renderer_set_dir(GAME_CHARACTER_DIR_LEFT);
        }
        if (game_input_right()) {
          next_x += 2;
          game_character_renderer_set_dir(GAME_CHARACTER_DIR_RIGHT);
        }

        if (next_x != player_x || next_y != player_y) {
          int16_t cx = next_x;
          int16_t cy = player_y;
          if (!is_blocked_explore(cx, cy)) {
            player_x = cx;
            moved = true;
          }
          cx = player_x;
          cy = next_y;
          if (!is_blocked_explore(cx, cy)) {
            player_y = cy;
            moved = true;
          }
        }
      }

      bool near = is_near_anvil(player_x, player_y);
      if (near && game_input_button_a_edge()) {
        mode = FORGE_MODE_CRAFT;
        craft_selected = 0;
        need_full = true;
      }

      if (moved && !need_full) {
        // Partial redraw: erase old player with correct floor texture,
        // then re-draw any intersected static objects.
        int16_t dx = player_x - prev_player_x;
        int16_t dy = player_y - prev_player_y;
        if (dx > 0) {
          redraw_explore_patch(prev_player_x - 1, prev_player_y - 1, dx + 2, PLAYER_H + 4, unlocked_mask);
        } else if (dx < 0) {
          redraw_explore_patch(player_x + PLAYER_W, prev_player_y - 1, -dx + 2, PLAYER_H + 4, unlocked_mask);
        }
        if (dy > 0) {
          redraw_explore_patch(prev_player_x - 1, prev_player_y - 1, PLAYER_W + 4, dy + 2, unlocked_mask);
        } else if (dy < 0) {
          redraw_explore_patch(prev_player_x - 1, player_y + PLAYER_H, PLAYER_W + 4, -dy + 2, unlocked_mask);
        }
        // Erase old shadow (not covered by above strips when moving vertically)
        redraw_explore_patch(prev_player_x + 4, prev_player_y + PLAYER_H - 2, 8, 3, unlocked_mask);
        // Draw new shadow + player
        game_lcd_fill_rect(player_x + 4, player_y + PLAYER_H - 2, 8, 3, 0x0841);
        game_character_renderer_draw(player_x, player_y);
        prev_player_x = player_x;
        prev_player_y = player_y;
      }
      break;
    }

    case FORGE_MODE_CRAFT: {
      bool up_now = game_input_up();
      bool down_now = game_input_down();
      uint8_t old_sel = craft_selected;
      bool changed = false;
      if (up_now && !prev_up) {
        if (craft_selected > 0) {
          craft_selected--;
          changed = true;
        }
      }
      if (down_now && !prev_down) {
        if (craft_selected < WEAPON_COUNT - 1) {
          craft_selected++;
          changed = true;
        }
      }
      prev_up = up_now;
      prev_down = down_now;
      if (changed) {
        // Partial redraw: only update the two affected items
        draw_craft_item(old_sel, false, unlocked_mask);
        draw_craft_item(craft_selected, true, unlocked_mask);
      }
      if (game_input_button_a_edge()) {
        mode = FORGE_MODE_CRAFTING;
        craft_start_ms = game_platform_millis();
        hammer_last_ms = 0;
        need_full = true;
      }
      break;
    }

    case FORGE_MODE_CRAFTING: {
      uint32_t elapsed = game_platform_millis() - craft_start_ms;
      play_hammer_sound(&hammer_last_ms);

      if (elapsed >= FORGE_TIME_MS) {
        mode = FORGE_MODE_CRAFT_DONE;
        melody_idx = 0;
        melody_note_ms = 0;
        if (shared != 0) {
          shared->weapon_unlocked_mask |= WEAPON_UNLOCK_BIT(craft_selected);
        }
        need_full = true;
      } else {
        // Partial: redraw only progress / sparks
        draw_crafting_anim(elapsed, unlocked_mask);
      }
      break;
    }

    case FORGE_MODE_CRAFT_DONE: {
      play_forge_success_melody(&melody_idx, &melody_note_ms);
      if (game_input_button_a_edge()) {
        mode = FORGE_MODE_CRAFT;
        game_buzzer_stop();
        need_full = true;
      }
      break;
    }
  }

  if (need_full) {
    if (mode == FORGE_MODE_EXPLORE) {
      draw_explore_scene(player_x, player_y, is_near_anvil(player_x, player_y), unlocked_mask);
      prev_player_x = player_x;
      prev_player_y = player_y;
    } else if (mode == FORGE_MODE_CRAFT) {
      draw_explore_scene(player_x, player_y, false, unlocked_mask);
      draw_craft_menu(craft_selected, unlocked_mask);
    } else if (mode == FORGE_MODE_CRAFTING) {
      draw_explore_scene(player_x, player_y, false, unlocked_mask);
      draw_craft_menu(craft_selected, unlocked_mask);
      draw_crafting_anim(game_platform_millis() - craft_start_ms, unlocked_mask);
    } else if (mode == FORGE_MODE_CRAFT_DONE) {
      draw_explore_scene(player_x, player_y, false, unlocked_mask);
      draw_craft_menu(craft_selected, unlocked_mask);
      draw_craft_done(s_weapon_names[craft_selected]);
    }
  }
}

void game_forge_scene_request_full_redraw(void) {
  s_forge_force_full_redraw = true;
}
