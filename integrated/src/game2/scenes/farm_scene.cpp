#include "game2/scenes/farm_scene.h"

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

#define FARM_GRID_COLS 3
#define FARM_GRID_ROWS 3
#define FARM_TILE_W 46
#define FARM_TILE_H 46
#define FARM_GRID_X 26
#define FARM_GRID_Y 42
#define FARM_STEP_X 62
#define FARM_STEP_Y 50
#define FARM_GROUND_COLOR COLOR_SOIL_LIGHT
#define FARM_PANEL_COLOR 0x2104
#define FARM_SELECT_RADIUS_SQ (44 * 44)
#define FARM_REACTION_BOX_X 18
#define FARM_REACTION_BOX_Y 114
#define FARM_REACTION_BOX_W 204
#define FARM_REACTION_BOX_H 52
#define FARM_REACTION_BAR_X (FARM_REACTION_BOX_X + 12)
#define FARM_REACTION_BAR_Y (FARM_REACTION_BOX_Y + 24)
#define FARM_REACTION_BAR_W (FARM_REACTION_BOX_W - 24)
#define FARM_REACTION_BAR_H 14
#define FARM_REACTION_TARGET_W 34
#define FARM_REACTION_LINE_W 2
#define FARM_REACTION_STEP_PX 7
#define FARM_REACTION_STEP_MS 24u

#define FARM_ANIM_WATER_MS 800u
#define FARM_ANIM_HARVEST_MS 600u
#define FARM_RENDER_INTERVAL_MS 16u
#define FARM_HUD_REFRESH_MS 1000u

#define FARM_COLOR_REACTION_WATER_BG 0x4AAA
#define FARM_COLOR_REACTION_FERT_BG 0x8B23
#define FARM_COLOR_REACTION_BAR 0x18E3
#define FARM_COLOR_REACTION_TARGET 0x8600
#define FARM_COLOR_REACTION_LINE 0xF800

#define COLOR_SOIL_LIGHT 0x8A62
#define COLOR_SOIL_DARK  0x6241
#define COLOR_SPROUT     0x07E0
#define COLOR_SPROUT_DK  0x04A0
#define COLOR_WHEAT      0xFFE0
#define COLOR_WHEAT_DK   0xFDA0
#define COLOR_KETTLE     0x9CD3
#define COLOR_KETTLE_DK  0x7BEF
#define COLOR_WATER      0x04BF
#define COLOR_SICKLE     0xCE79
#define COLOR_SICKLE_H   0x8A62
#define COLOR_FENCE      0xCE59
#define COLOR_FENCE_DK   0x8A62

typedef enum {
  FARM_TILE_EMPTY = 0,
  FARM_TILE_SEEDED,
  FARM_TILE_WATERED,
  FARM_TILE_FERTILIZED,
  FARM_TILE_READY
} farm_tile_state_t;

typedef struct {
  farm_tile_state_t state;
  uint32_t state_since_ms;
} farm_tile_t;

typedef enum {
  FARM_REACTION_NONE = 0,
  FARM_REACTION_WATER,
  FARM_REACTION_FERTILIZE
} farm_reaction_mode_t;

typedef enum {
  FARM_ANIM_NONE = 0,
  FARM_ANIM_WATERING,
  FARM_ANIM_HARVESTING
} farm_anim_t;

static const char* action_hint_for_state(farm_tile_state_t state);

static bool s_farm_force_full_redraw = false;
static bool s_farm_need_full_redraw = false;

// ---------------------------------------------------------------------------
// Sound effects
// ---------------------------------------------------------------------------
static void play_harvest_success_melody(uint8_t* idx, uint32_t* note_ms) {
  static const uint16_t freqs[] = {659, 831, 988, 1319, 0};
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

static int16_t farm_tile_x(uint8_t col) { return (int16_t)(FARM_GRID_X + col * FARM_STEP_X); }
static int16_t farm_tile_y(uint8_t row) { return (int16_t)(FARM_GRID_Y + row * FARM_STEP_Y); }

static void draw_sprout_small(int16_t tx, int16_t ty) {
  int16_t cx = (int16_t)(tx + 22);
  int16_t by = (int16_t)(ty + 38);
  game_lcd_fill_rect(cx, (int16_t)(by - 6), 2, 7, COLOR_SPROUT);
  game_lcd_fill_rect((int16_t)(cx - 1), (int16_t)(by - 4), 2, 2, COLOR_SPROUT_DK);
  game_lcd_fill_rect((int16_t)(cx + 1), (int16_t)(by - 3), 2, 2, COLOR_SPROUT_DK);
}

static void draw_sprout_medium(int16_t tx, int16_t ty) {
  int16_t cx = (int16_t)(tx + 22);
  int16_t by = (int16_t)(ty + 38);
  game_lcd_fill_rect(cx, (int16_t)(by - 10), 2, 11, COLOR_SPROUT);
  game_lcd_fill_rect((int16_t)(cx - 3), (int16_t)(by - 7), 3, 3, COLOR_SPROUT);
  game_lcd_fill_rect((int16_t)(cx + 2), (int16_t)(by - 8), 3, 3, COLOR_SPROUT);
  game_lcd_fill_rect((int16_t)(cx - 1), (int16_t)(by - 4), 2, 2, COLOR_SPROUT_DK);
  game_lcd_fill_rect((int16_t)(cx + 1), (int16_t)(by - 3), 2, 2, COLOR_SPROUT_DK);
}

static void draw_wheat_small(int16_t tx, int16_t ty) {
  int16_t cx = (int16_t)(tx + 22);
  int16_t by = (int16_t)(ty + 38);
  game_lcd_fill_rect(cx, (int16_t)(by - 12), 2, 13, COLOR_WHEAT);
  game_lcd_fill_rect((int16_t)(cx - 2), (int16_t)(by - 14), 6, 4, COLOR_WHEAT);
  game_lcd_fill_rect((int16_t)(cx - 1), (int16_t)(by - 16), 4, 3, COLOR_WHEAT);
  game_lcd_fill_rect((int16_t)(cx - 3), (int16_t)(by - 8), 3, 2, COLOR_WHEAT_DK);
  game_lcd_fill_rect((int16_t)(cx + 2), (int16_t)(by - 9), 3, 2, COLOR_WHEAT_DK);
}

static void draw_wheat_large(int16_t tx, int16_t ty) {
  int16_t cx = (int16_t)(tx + 22);
  int16_t by = (int16_t)(ty + 38);
  game_lcd_fill_rect(cx, (int16_t)(by - 18), 2, 19, COLOR_WHEAT);
  game_lcd_fill_rect((int16_t)(cx - 3), (int16_t)(by - 20), 8, 5, COLOR_WHEAT);
  game_lcd_fill_rect((int16_t)(cx - 2), (int16_t)(by - 23), 6, 4, COLOR_WHEAT);
  game_lcd_fill_rect((int16_t)(cx - 1), (int16_t)(by - 25), 4, 3, COLOR_WHEAT);
  game_lcd_fill_rect((int16_t)(cx - 4), (int16_t)(by - 12), 3, 3, COLOR_WHEAT_DK);
  game_lcd_fill_rect((int16_t)(cx + 3), (int16_t)(by - 14), 3, 3, COLOR_WHEAT_DK);
  game_lcd_fill_rect((int16_t)(cx - 3), (int16_t)(by - 6), 3, 2, COLOR_WHEAT_DK);
  game_lcd_fill_rect((int16_t)(cx + 2), (int16_t)(by - 8), 3, 2, COLOR_WHEAT_DK);
}

static void draw_fence(void) {
  int16_t fx = (int16_t)(FARM_GRID_X - 4);
  int16_t fy = (int16_t)(FARM_GRID_Y - 4);
  int16_t fw = (int16_t)(FARM_STEP_X * 2 + FARM_TILE_W + 8);
  int16_t fh = (int16_t)(FARM_STEP_Y * 2 + FARM_TILE_H + 8);

  game_lcd_fill_rect(fx, fy, 4, fh, COLOR_FENCE);
  game_lcd_fill_rect((int16_t)(fx + fw - 4), fy, 4, fh, COLOR_FENCE);
  game_lcd_fill_rect(fx, fy, fw, 4, COLOR_FENCE);
  game_lcd_fill_rect(fx, (int16_t)(fy + fh - 4), fw, 4, COLOR_FENCE);

  game_lcd_fill_rect(79, fy, 3, fh, COLOR_FENCE_DK);
  game_lcd_fill_rect(141, fy, 3, fh, COLOR_FENCE_DK);

  game_lcd_fill_rect(fx, 89, fw, 2, COLOR_FENCE);
  game_lcd_fill_rect(fx, 139, fw, 2, COLOR_FENCE);
}

static void draw_watering_anim(int16_t px, int16_t py, uint32_t elapsed_ms) {
  int16_t kx = (int16_t)(px + 14);
  int16_t ky = (int16_t)(py - 4);

  // Kettle body
  game_lcd_fill_rect(kx, ky, 10, 8, COLOR_KETTLE);
  game_lcd_draw_rect(kx, ky, 10, 8, COLOR_KETTLE_DK);
  game_lcd_fill_rect((int16_t)(kx - 3), (int16_t)(ky + 1), 4, 6, COLOR_KETTLE);

  if (elapsed_ms < 200) {
    // Tilt up
    game_lcd_fill_rect((int16_t)(kx + 8), (int16_t)(ky + 2), 6, 3, COLOR_KETTLE);
  } else if (elapsed_ms < 600) {
    // Pouring pose
    game_lcd_fill_rect((int16_t)(kx + 7), (int16_t)(ky + 4), 5, 4, COLOR_KETTLE);
    game_lcd_fill_rect((int16_t)(kx + 10), (int16_t)(ky + 7), 4, 3, COLOR_KETTLE);

    uint16_t dt = (uint16_t)(elapsed_ms - 200);
    // 4 water drops with staggered starts and speeds
    int16_t d1y = (int16_t)(ky + 10 + (int16_t)(dt / 18));
    if (d1y < py + 18) game_lcd_fill_rect((int16_t)(kx + 11), d1y, 2, 2, COLOR_WATER);
    if (dt > 60) {
      int16_t d2y = (int16_t)(ky + 10 + (int16_t)((dt - 60) / 22));
      if (d2y < py + 18) game_lcd_fill_rect((int16_t)(kx + 13), d2y, 2, 2, 0x03F8);
    }
    if (dt > 140) {
      int16_t d3y = (int16_t)(ky + 10 + (int16_t)((dt - 140) / 26));
      if (d3y < py + 18) game_lcd_fill_rect((int16_t)(kx + 9), d3y, 2, 2, COLOR_WATER);
    }
    if (dt > 220) {
      int16_t d4y = (int16_t)(ky + 10 + (int16_t)((dt - 220) / 30));
      if (d4y < py + 18) game_lcd_fill_rect((int16_t)(kx + 12), d4y, 2, 2, 0x03F8);
    }
    // Ground splash
    if (dt > 180 && dt < 400) {
      int16_t sp = (int16_t)((dt - 180) / 30);
      if (sp < 5) {
        game_lcd_fill_rect((int16_t)(kx + 7 + sp), (int16_t)(py + 15), 2, 1, 0x03F8);
        game_lcd_fill_rect((int16_t)(kx + 14 - sp), (int16_t)(py + 16), 2, 1, COLOR_WATER);
      }
    }
  } else {
    // Put down
    game_lcd_fill_rect((int16_t)(kx + 8), (int16_t)(ky + 2), 6, 3, COLOR_KETTLE);
  }
}

static void draw_harvest_anim(int16_t tx, int16_t ty, uint32_t elapsed_ms) {
  int16_t progress = (int16_t)(elapsed_ms * (FARM_TILE_W + 16) / FARM_ANIM_HARVEST_MS);
  int16_t sx = (int16_t)(tx - 8 + progress);
  int16_t sy = (int16_t)(ty + 10);

  // Sickle body
  game_lcd_fill_rect((int16_t)(sx + 4), sy, 10, 3, COLOR_SICKLE);
  game_lcd_fill_rect((int16_t)(sx + 6), (int16_t)(sy - 2), 8, 2, COLOR_SICKLE);
  game_lcd_fill_rect((int16_t)(sx + 8), (int16_t)(sy - 3), 5, 2, COLOR_SICKLE);
  game_lcd_fill_rect((int16_t)(sx - 4), (int16_t)(sy + 1), 9, 2, COLOR_SICKLE_H);

  // Cutting sparkle
  if (elapsed_ms < 500 && (elapsed_ms % 120) < 40) {
    game_lcd_fill_rect((int16_t)(sx + 10), (int16_t)(sy - 2), 2, 2, GAME_REF_COLOR_WHITE);
  }

  // Wheat debris inside tile bounds
  if (elapsed_ms > 80 && elapsed_ms < 520) {
    uint16_t pt = (uint16_t)(elapsed_ms - 80);
    int16_t p1x = (int16_t)(tx + 12 + (pt % 24) / 6);
    int16_t p1y = (int16_t)(ty + 18 - pt / 28);
    if (p1y > ty + 4 && p1x < tx + FARM_TILE_W - 2)
      game_lcd_fill_rect(p1x, p1y, 2, 2, COLOR_WHEAT);
    int16_t p2x = (int16_t)(tx + 28 - (pt % 20) / 5);
    int16_t p2y = (int16_t)(ty + 22 - pt / 22);
    if (p2y > ty + 6 && p2x > tx + 4)
      game_lcd_fill_rect(p2x, p2y, 2, 2, COLOR_WHEAT_DK);
  }
}

static void draw_tile_bg_only(int16_t x, int16_t y, farm_tile_t tile) {
  game_lcd_fill_rect(x, y, FARM_TILE_W, FARM_TILE_H, COLOR_SOIL_LIGHT);
  switch (tile.state) {
    case FARM_TILE_SEEDED:
      draw_sprout_small(x, y);
      break;
    case FARM_TILE_WATERED:
      draw_sprout_medium(x, y);
      break;
    case FARM_TILE_FERTILIZED:
      draw_wheat_small(x, y);
      break;
    case FARM_TILE_READY:
      draw_wheat_large(x, y);
      break;
    default:
      break;
  }
}

static void draw_tile_overlay(int16_t x, int16_t y, farm_tile_t tile, bool selected) {
  uint16_t border = selected ? GAME_REF_COLOR_CYAN : 0x7BEF;

  game_lcd_draw_rect(x, y, FARM_TILE_W, FARM_TILE_H, border);

  game_lcd_fill_rect((int16_t)(x + 8), (int16_t)(y + 14), (int16_t)(FARM_TILE_W - 16), 1, COLOR_SOIL_DARK);
  game_lcd_fill_rect((int16_t)(x + 8), (int16_t)(y + 30), (int16_t)(FARM_TILE_W - 16), 1, COLOR_SOIL_DARK);
  game_lcd_fill_rect((int16_t)(x + 14), (int16_t)(y + 8), 1, (int16_t)(FARM_TILE_H - 16), COLOR_SOIL_DARK);
  game_lcd_fill_rect((int16_t)(x + 30), (int16_t)(y + 8), 1, (int16_t)(FARM_TILE_H - 16), COLOR_SOIL_DARK);

  switch (tile.state) {
    case FARM_TILE_SEEDED:
      draw_sprout_small(x, y);
      break;
    case FARM_TILE_WATERED:
      draw_sprout_medium(x, y);
      break;
    case FARM_TILE_FERTILIZED:
      draw_wheat_small(x, y);
      break;
    case FARM_TILE_READY:
      draw_wheat_large(x, y);
      break;
    default:
      break;
  }
}

static void draw_tile(int16_t x, int16_t y, farm_tile_t tile, bool selected) {
  game_lcd_fill_rect(x, y, FARM_TILE_W, FARM_TILE_H, COLOR_SOIL_LIGHT);
  draw_tile_overlay(x, y, tile, selected);
}

static void draw_tile_by_index(uint8_t tile_index, const farm_tile_t* tiles, int8_t selected_index) {
  uint8_t row = (uint8_t)(tile_index / FARM_GRID_COLS);
  uint8_t col = (uint8_t)(tile_index % FARM_GRID_COLS);
  int16_t x = farm_tile_x(col);
  int16_t y = farm_tile_y(row);
  bool selected = (selected_index >= 0 && tile_index == (uint8_t)selected_index);
  draw_tile(x, y, tiles[tile_index], selected);
}

static void draw_player_in_farm(int16_t px, int16_t py, bool moving) {
  int16_t shadow_w = moving ? 6 : 8;
  int16_t shadow_h = moving ? 2 : 3;
  int16_t shadow_x = (int16_t)(px + (moving ? 5 : 4));
  int16_t shadow_y = (int16_t)(py + GAME_REF_CHARACTER_RENDER_SIZE - (moving ? 1 : 2));

  game_lcd_fill_rect(shadow_x, shadow_y, shadow_w, shadow_h, 0x0841);
  game_character_renderer_draw(px, py);
}

static int8_t nearest_tile_index(int16_t player_x, int16_t player_y) {
  int16_t cx = (int16_t)(player_x + GAME_REF_CHARACTER_RENDER_SIZE / 2);
  int16_t cy = (int16_t)(player_y + GAME_REF_CHARACTER_RENDER_SIZE / 2);
  int32_t best_dist = 0x7FFFFFFF;
  int8_t best_idx = -1;
  uint8_t row;
  uint8_t col;

  for (row = 0; row < FARM_GRID_ROWS; row++) {
    for (col = 0; col < FARM_GRID_COLS; col++) {
      uint8_t idx = (uint8_t)(row * FARM_GRID_COLS + col);
      int16_t tx = (int16_t)(farm_tile_x(col) + FARM_TILE_W / 2);
      int16_t ty = (int16_t)(farm_tile_y(row) + FARM_TILE_H / 2);
      int32_t dx = (int32_t)(cx - tx);
      int32_t dy = (int32_t)(cy - ty);
      int32_t d2 = dx * dx + dy * dy;

      if (d2 < best_dist) {
        best_dist = d2;
        best_idx = (int8_t)idx;
      }
    }
  }

  if (best_dist <= FARM_SELECT_RADIUS_SQ) {
    return best_idx;
  }
  return -1;
}

static void draw_farm_hud(int8_t selected_idx,
                          const farm_tile_t* tiles,
                          const char* status_line,
                          uint16_t food,
                          uint16_t metal) {
  char food_line[20];
  char metal_line[20];
  const char* action = "MOVE CLOSER";

  if (selected_idx >= 0) {
    action = action_hint_for_state(tiles[selected_idx].state);
  }

  snprintf(food_line, sizeof(food_line), "FOOD:%u", (unsigned)food);
  snprintf(metal_line, sizeof(metal_line), "METAL:%u", (unsigned)metal);
  game_lcd_fill_rect(122, 2, 116, 20, GAME_REF_COLOR_BLACK);
  game_lcd_draw_text(124, 3, food_line, GAME_REF_COLOR_YELLOW, GAME_REF_COLOR_BLACK, 1);
  game_lcd_draw_text(124, 12, metal_line, GAME_REF_COLOR_CYAN, GAME_REF_COLOR_BLACK, 1);

  game_lcd_fill_rect(0, 200, GAME_SCREEN_WIDTH, 40, FARM_PANEL_COLOR);
  game_lcd_draw_rect(0, 200, GAME_SCREEN_WIDTH, 40, GAME_REF_COLOR_WHITE);
  game_lcd_draw_text(8, 206, action, GAME_REF_COLOR_YELLOW, FARM_PANEL_COLOR, 1);
  game_lcd_draw_text(104, 206, "B:RETURN", GAME_REF_COLOR_CYAN, FARM_PANEL_COLOR, 1);
  game_lcd_draw_text(8, 218, status_line, GAME_REF_COLOR_WHITE, FARM_PANEL_COLOR, 1);
}

static uint16_t reaction_frame_bg_color(farm_reaction_mode_t mode) {
  return (mode == FARM_REACTION_FERTILIZE) ? FARM_COLOR_REACTION_FERT_BG : FARM_COLOR_REACTION_WATER_BG;
}

static const char* reaction_title(farm_reaction_mode_t mode) {
  return (mode == FARM_REACTION_FERTILIZE) ? "FERTILIZE STEP" : "WATER STEP";
}

static bool reaction_x_in_target(int16_t x) {
  int16_t target_x0 = (int16_t)(FARM_REACTION_BAR_X + (FARM_REACTION_BAR_W - FARM_REACTION_TARGET_W) / 2);
  int16_t target_x1 = (int16_t)(target_x0 + FARM_REACTION_TARGET_W - 1);
  return x >= target_x0 && x <= target_x1;
}

static void draw_reaction_frame(farm_reaction_mode_t mode) {
  uint16_t bg = FARM_COLOR_REACTION_WATER_BG;
  const char* title = "WATER STEP";
  int16_t target_x = (int16_t)(FARM_REACTION_BAR_X + (FARM_REACTION_BAR_W - FARM_REACTION_TARGET_W) / 2);

  bg = reaction_frame_bg_color(mode);
  title = reaction_title(mode);

  game_lcd_fill_rect(FARM_REACTION_BOX_X, FARM_REACTION_BOX_Y, FARM_REACTION_BOX_W, FARM_REACTION_BOX_H, bg);
  game_lcd_draw_rect(FARM_REACTION_BOX_X,
                     FARM_REACTION_BOX_Y,
                     FARM_REACTION_BOX_W,
                     FARM_REACTION_BOX_H,
                     GAME_REF_COLOR_WHITE);

  game_lcd_draw_text((int16_t)(FARM_REACTION_BOX_X + 8),
                     (int16_t)(FARM_REACTION_BOX_Y + 7),
                     title,
                     GAME_REF_COLOR_WHITE,
                     bg,
                     1);
  game_lcd_draw_text((int16_t)(FARM_REACTION_BOX_X + 110),
                     (int16_t)(FARM_REACTION_BOX_Y + 7),
                     "A:STOP",
                     GAME_REF_COLOR_YELLOW,
                     bg,
                     1);

  game_lcd_fill_rect(FARM_REACTION_BAR_X,
                     FARM_REACTION_BAR_Y,
                     FARM_REACTION_BAR_W,
                     FARM_REACTION_BAR_H,
                     FARM_COLOR_REACTION_BAR);
  game_lcd_draw_rect(FARM_REACTION_BAR_X,
                     FARM_REACTION_BAR_Y,
                     FARM_REACTION_BAR_W,
                     FARM_REACTION_BAR_H,
                     GAME_REF_COLOR_WHITE);

  game_lcd_fill_rect(target_x,
                     (int16_t)(FARM_REACTION_BAR_Y + 1),
                     FARM_REACTION_TARGET_W,
                     (int16_t)(FARM_REACTION_BAR_H - 2),
                     FARM_COLOR_REACTION_TARGET);
  game_lcd_draw_rect(target_x,
                     (int16_t)(FARM_REACTION_BAR_Y + 1),
                     FARM_REACTION_TARGET_W,
                     (int16_t)(FARM_REACTION_BAR_H - 2),
                     GAME_REF_COLOR_WHITE);

}

static void draw_reaction_line(int16_t line_x) {
  game_lcd_fill_rect(line_x,
                     (int16_t)(FARM_REACTION_BAR_Y - 2),
                     FARM_REACTION_LINE_W,
                     (int16_t)(FARM_REACTION_BAR_H + 4),
                     FARM_COLOR_REACTION_LINE);
}

static void erase_reaction_line(farm_reaction_mode_t mode, int16_t line_x) {
  uint16_t frame_bg = reaction_frame_bg_color(mode);
  bool all_target = reaction_x_in_target(line_x) && reaction_x_in_target(line_x + FARM_REACTION_LINE_W - 1);
  bool all_bar = !reaction_x_in_target(line_x) && !reaction_x_in_target(line_x + FARM_REACTION_LINE_W - 1);
  uint16_t mid_fill;

  if (all_target) {
    mid_fill = FARM_COLOR_REACTION_TARGET;
  } else if (all_bar) {
    mid_fill = FARM_COLOR_REACTION_BAR;
  } else {
    // Mixed (rare with LINE_W=3): fall back to per-column
    int16_t dy;
    for (dy = 0; dy < FARM_REACTION_LINE_W; dy++) {
      int16_t x = (int16_t)(line_x + dy);
      uint16_t bar_fill = reaction_x_in_target(x) ? FARM_COLOR_REACTION_TARGET : FARM_COLOR_REACTION_BAR;
      game_lcd_fill_rect(x, (int16_t)(FARM_REACTION_BAR_Y - 2), 1, 2, frame_bg);
      game_lcd_fill_rect(x, FARM_REACTION_BAR_Y, 1, 1, GAME_REF_COLOR_WHITE);
      game_lcd_fill_rect(x, (int16_t)(FARM_REACTION_BAR_Y + 1), 1, (int16_t)(FARM_REACTION_BAR_H - 2), bar_fill);
      game_lcd_fill_rect(x, (int16_t)(FARM_REACTION_BAR_Y + FARM_REACTION_BAR_H - 1), 1, 1, GAME_REF_COLOR_WHITE);
      game_lcd_fill_rect(x, (int16_t)(FARM_REACTION_BAR_Y + FARM_REACTION_BAR_H), 1, 2, frame_bg);
    }
    return;
  }

  game_lcd_fill_rect(line_x, (int16_t)(FARM_REACTION_BAR_Y - 2), FARM_REACTION_LINE_W, 2, frame_bg);
  game_lcd_fill_rect(line_x, FARM_REACTION_BAR_Y, FARM_REACTION_LINE_W, 1, GAME_REF_COLOR_WHITE);
  game_lcd_fill_rect(line_x, (int16_t)(FARM_REACTION_BAR_Y + 1), FARM_REACTION_LINE_W, (int16_t)(FARM_REACTION_BAR_H - 2), mid_fill);
  game_lcd_fill_rect(line_x, (int16_t)(FARM_REACTION_BAR_Y + FARM_REACTION_BAR_H - 1), FARM_REACTION_LINE_W, 1, GAME_REF_COLOR_WHITE);
  game_lcd_fill_rect(line_x, (int16_t)(FARM_REACTION_BAR_Y + FARM_REACTION_BAR_H), FARM_REACTION_LINE_W, 2, frame_bg);
}

static bool reaction_hit_target(int16_t line_x) {
  int16_t target_x0 = (int16_t)(FARM_REACTION_BAR_X + (FARM_REACTION_BAR_W - FARM_REACTION_TARGET_W) / 2);
  int16_t target_x1 = (int16_t)(target_x0 + FARM_REACTION_TARGET_W - 1);
  int16_t line_center = (int16_t)(line_x + FARM_REACTION_LINE_W / 2);
  return (line_center >= target_x0 && line_center <= target_x1);
}

static void redraw_farm_patch(int16_t x,
                              int16_t y,
                              int16_t w,
                              int16_t h,
                              const farm_tile_t* tiles,
                              int8_t selected_idx) {
  int16_t sx = x;
  int16_t sy = y;
  int16_t ex = (int16_t)(x + w - 1);
  int16_t ey = (int16_t)(y + h - 1);
  uint8_t idx;

  if (sx < 1) sx = 1;
  if (sy < GAME_STATUS_BAR_HEIGHT + 1) sy = GAME_STATUS_BAR_HEIGHT + 1;
  if (ex > GAME_SCREEN_WIDTH - 2) ex = GAME_SCREEN_WIDTH - 2;
  if (ey > 199) ey = 199;
  if (sx > ex || sy > ey) return;

  game_lcd_fill_rect(sx, sy, (int16_t)(ex - sx + 1), (int16_t)(ey - sy + 1), FARM_GROUND_COLOR);

  for (idx = 0; idx < 9; idx++) {
    uint8_t row = (uint8_t)(idx / FARM_GRID_COLS);
    uint8_t col = (uint8_t)(idx % FARM_GRID_COLS);
    int16_t tx = farm_tile_x(col);
    int16_t ty = farm_tile_y(row);
    if (!(tx + FARM_TILE_W <= sx || ex + 1 <= tx || ty + FARM_TILE_H <= sy || ey + 1 <= ty)) {
      bool selected = (selected_idx >= 0 && idx == (uint8_t)selected_idx);
      draw_tile_overlay(tx, ty, tiles[idx], selected);
    }
  }

  {
    int16_t fence_x = (int16_t)(FARM_GRID_X - 4);
    int16_t fence_y = (int16_t)(FARM_GRID_Y - 4);
    int16_t fence_w = (int16_t)(FARM_STEP_X * 2 + FARM_TILE_W + 8);
    int16_t fence_h = (int16_t)(FARM_STEP_Y * 2 + FARM_TILE_H + 8);
    if (!(sx >= fence_x + fence_w || ex + 1 <= fence_x ||
          sy >= fence_y + fence_h || ey + 1 <= fence_y)) {
      draw_fence();
    }
  }
}

static const char* action_hint_for_state(farm_tile_state_t state) {
  switch (state) {
    case FARM_TILE_EMPTY:
      return "A:SOW";
    case FARM_TILE_SEEDED:
      return "A:WATER";
    case FARM_TILE_WATERED:
      return "A:FERTILIZE";
    case FARM_TILE_FERTILIZED:
      return "GROWING";
    case FARM_TILE_READY:
      return "A:HARVEST";
    default:
      return "";
  }
}


static void clear_reaction_frame(const farm_tile_t* tiles, int8_t selected_idx) {
  redraw_farm_patch(FARM_REACTION_BOX_X - 2, FARM_REACTION_BOX_Y - 2,
                    FARM_REACTION_BOX_W + 4, FARM_REACTION_BOX_H + 4,
                    tiles, selected_idx);
}

static void sync_tiles_to_shared(const farm_tile_t* tiles) {
  game_shared_resources_t* shared = game_app_shared_resources();
  if (shared != 0) {
    uint8_t i;
    for (i = 0; i < 9; i++) {
      shared->farm_tile_state[i] = (uint8_t)tiles[i].state;
      shared->farm_tile_since[i] = tiles[i].state_since_ms;
    }
  }
}

static void load_tiles_from_shared(farm_tile_t* tiles) {
  game_shared_resources_t* shared = game_app_shared_resources();
  if (shared != 0) {
    uint8_t i;
    for (i = 0; i < 9; i++) {
      tiles[i].state = (farm_tile_state_t)shared->farm_tile_state[i];
      tiles[i].state_since_ms = shared->farm_tile_since[i];
    }
  } else {
    uint8_t i;
    for (i = 0; i < 9; i++) {
      tiles[i].state = FARM_TILE_EMPTY;
      tiles[i].state_since_ms = 0;
    }
  }
}

void game_farm_scene_update(void) {
  static bool initialized = false;
  static farm_tile_t tiles[9];
  static int16_t player_x = 112;
  static int16_t player_y = 168;
  static int16_t prev_player_x = 112;
  static int16_t prev_player_y = 168;
  static int8_t selected_idx = -1;
  static int8_t last_selected_idx = -1;
  static farm_reaction_mode_t reaction_mode = FARM_REACTION_NONE;
  static uint8_t reaction_tile_idx = 0;
  static int16_t reaction_line_x = FARM_REACTION_BAR_X;
  static int16_t reaction_prev_line_x = FARM_REACTION_BAR_X;
  static int8_t reaction_dir = 1;
  static uint32_t reaction_last_step_ms = 0;
  static uint32_t last_move_ms = 0;
  static uint32_t last_render_ms = 0;
  static char status_line[24] = "FARM READY";
  static farm_anim_t anim_mode = FARM_ANIM_NONE;
  static uint8_t anim_tile_idx = 0;
  static uint32_t anim_start_ms = 0;
  static int16_t anim_prev_x = 0;
  static int16_t anim_prev_y = 0;
  static int16_t anim_prev_w = 0;
  static int16_t anim_prev_h = 0;
  static uint8_t harvest_melody_idx = 0;
  static uint32_t harvest_melody_ms = 0;
  static int8_t changed_tile_idx = -1;

  uint8_t idx;
  bool moved = false;
  bool state_changed = false;
  bool need_partial_render = false;
  bool hud_dirty = false;
  uint32_t now = game_platform_millis();
  bool can_render = (now - last_render_ms >= FARM_RENDER_INTERVAL_MS);
  game_shared_resources_t* shared;

  if (s_farm_force_full_redraw) {
    initialized = false;
    s_farm_force_full_redraw = false;
  }

  if (!initialized) {
    load_tiles_from_shared(tiles);
    game_character_renderer_init();
    selected_idx = nearest_tile_index(player_x, player_y);
    last_selected_idx = selected_idx;
    reaction_mode = FARM_REACTION_NONE;
    anim_mode = FARM_ANIM_NONE;
    changed_tile_idx = -1;
    harvest_melody_idx = 0;
    harvest_melody_ms = 0;
    game_buzzer_stop();
    initialized = true;
    s_farm_need_full_redraw = true;
  }

  game_input_update();

  if (reaction_mode == FARM_REACTION_NONE && anim_mode == FARM_ANIM_NONE &&
      now - last_move_ms >= 16u) {
    int16_t next_x = player_x;
    int16_t next_y = player_y;
    last_move_ms = now;

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

    if (next_x < GAME_PLAY_AREA_X) next_x = GAME_PLAY_AREA_X;
    if (next_y < GAME_STATUS_BAR_HEIGHT + 2) next_y = GAME_STATUS_BAR_HEIGHT + 2;
    if (next_x > GAME_SCREEN_WIDTH - GAME_REF_CHARACTER_RENDER_SIZE - 2)
      next_x = GAME_SCREEN_WIDTH - GAME_REF_CHARACTER_RENDER_SIZE - 2;
    if (next_y > 200 - GAME_REF_CHARACTER_RENDER_SIZE - 2)
      next_y = 200 - GAME_REF_CHARACTER_RENDER_SIZE - 2;

    if (next_x != player_x || next_y != player_y) {
      player_x = next_x;
      player_y = next_y;
      moved = true;
    }
  }

  selected_idx = nearest_tile_index(player_x, player_y);
  if (selected_idx != last_selected_idx) {
    hud_dirty = true;
  }

  // Tile growth update: fertilized tile becomes harvestable after a short wait.
  for (idx = 0; idx < 9; idx++) {
    if (tiles[idx].state == FARM_TILE_FERTILIZED &&
        now - tiles[idx].state_since_ms >= 6000u) {
      tiles[idx].state = FARM_TILE_READY;
      tiles[idx].state_since_ms = now;
      snprintf(status_line, sizeof(status_line), "PLOT %u READY", (unsigned)(idx + 1));
      state_changed = true;
      hud_dirty = true;
      changed_tile_idx = (int8_t)idx;
      sync_tiles_to_shared(tiles);
    }
  }

  // Animation state machine
  if (anim_mode == FARM_ANIM_WATERING) {
    if (now - anim_start_ms >= FARM_ANIM_WATER_MS) {
      // Final erase before state change
      if (anim_prev_w > 0) {
        game_lcd_fill_rect(anim_prev_x, anim_prev_y, anim_prev_w, anim_prev_h, FARM_GROUND_COLOR);
        draw_player_in_farm(player_x, player_y, false);
      }
      tiles[anim_tile_idx].state = FARM_TILE_WATERED;
      snprintf(status_line, sizeof(status_line), "WATERED PLOT %u", (unsigned)(anim_tile_idx + 1));
      game_buzzer_tone(700, 80);
      anim_mode = FARM_ANIM_NONE;
      state_changed = true;
      harvest_melody_idx = 0;
      harvest_melody_ms = 0;
      s_farm_need_full_redraw = true;
      hud_dirty = true;
      changed_tile_idx = (int8_t)anim_tile_idx;
      sync_tiles_to_shared(tiles);
      anim_prev_w = 0;
      // Ensure next normal frame renders immediately
      last_render_ms = now - FARM_RENDER_INTERVAL_MS;
    } else if (now - last_render_ms >= 80u) {
      int16_t old_x = anim_prev_x;
      int16_t old_y = anim_prev_y;
      int16_t old_w = anim_prev_w;
      int16_t old_h = anim_prev_h;

      // Fixed bounding box that fully covers kettle + all drops + splash
      int16_t new_x = (int16_t)(player_x + 6);
      int16_t new_y = (int16_t)(player_y - 6);
      int16_t new_w = 30;
      int16_t new_h = 30;

      int16_t ux = new_x;
      int16_t uy = new_y;
      int16_t uw = new_w;
      int16_t uh = new_h;

      if (old_w > 0) {
        int16_t right  = (int16_t)((old_x + old_w > new_x + new_w) ? old_x + old_w : new_x + new_w);
        int16_t bottom = (int16_t)((old_y + old_h > new_y + new_h) ? old_y + old_h : new_y + new_h);
        ux = (int16_t)((old_x < new_x) ? old_x : new_x);
        uy = (int16_t)((old_y < new_y) ? old_y : new_y);
        uw = (int16_t)(right - ux);
        uh = (int16_t)(bottom - uy);
      }

      // Light-weight erase: solid ground color only (no tile borders/crosses)
      game_lcd_fill_rect(ux, uy, uw, uh, FARM_GROUND_COLOR);
      // Restore the watered tile background + crop if intersected
      if (anim_tile_idx >= 0) {
        uint8_t wrow = (uint8_t)(anim_tile_idx / FARM_GRID_COLS);
        uint8_t wcol = (uint8_t)(anim_tile_idx % FARM_GRID_COLS);
        int16_t wtx = farm_tile_x(wcol);
        int16_t wty = farm_tile_y(wrow);
        if (!(wtx + FARM_TILE_W <= ux || ux + uw <= wtx || wty + FARM_TILE_H <= uy || uy + uh <= wty)) {
          draw_tile_bg_only(wtx, wty, tiles[anim_tile_idx]);
        }
      }

      draw_player_in_farm(player_x, player_y, false);
      draw_watering_anim(player_x, player_y, now - anim_start_ms);

      anim_prev_x = new_x;
      anim_prev_y = new_y;
      anim_prev_w = new_w;
      anim_prev_h = new_h;
      last_render_ms = now;
    }
  } else if (anim_mode == FARM_ANIM_HARVESTING) {
    if (now - anim_start_ms >= FARM_ANIM_HARVEST_MS) {
      // Final erase before state change
      if (anim_prev_w > 0) {
        game_lcd_fill_rect(anim_prev_x, anim_prev_y, anim_prev_w, anim_prev_h, FARM_GROUND_COLOR);
        draw_player_in_farm(player_x, player_y, false);
      }
      tiles[anim_tile_idx].state = FARM_TILE_EMPTY;
      shared = game_app_shared_resources();
      if (shared != 0) {
        shared->food = (uint16_t)(shared->food + 1);
        if (((now + anim_tile_idx) & 1u) == 0u) {
          shared->metal = (uint16_t)(shared->metal + 1);
          snprintf(status_line, sizeof(status_line), "HARVEST FOOD+METAL");
        } else {
          snprintf(status_line, sizeof(status_line), "HARVEST FOOD");
        }
      } else {
        snprintf(status_line, sizeof(status_line), "HARVEST FOOD");
      }
      anim_mode = FARM_ANIM_NONE;
      state_changed = true;
      s_farm_need_full_redraw = true;
      hud_dirty = true;
      changed_tile_idx = (int8_t)anim_tile_idx;
      sync_tiles_to_shared(tiles);
      anim_prev_w = 0;
      // Ensure next normal frame renders immediately
      last_render_ms = now - FARM_RENDER_INTERVAL_MS;
    } else if (now - last_render_ms >= 80u) {
      uint8_t arow = (uint8_t)(anim_tile_idx / FARM_GRID_COLS);
      uint8_t acol = (uint8_t)(anim_tile_idx % FARM_GRID_COLS);
      int16_t t_x = farm_tile_x(acol);
      int16_t t_y = farm_tile_y(arow);
      int16_t progress = (int16_t)((now - anim_start_ms) * (FARM_TILE_W + 16) / FARM_ANIM_HARVEST_MS);

      int16_t old_x = anim_prev_x;
      int16_t old_y = anim_prev_y;
      int16_t old_w = anim_prev_w;
      int16_t old_h = anim_prev_h;

      int16_t new_x = (int16_t)(t_x - 10 + progress);
      int16_t new_y = (int16_t)(t_y + 5);
      int16_t new_w = 26;
      int16_t new_h = 14;

      int16_t ux = new_x;
      int16_t uy = new_y;
      int16_t uw = new_w;
      int16_t uh = new_h;

      if (old_w > 0) {
        int16_t right  = (int16_t)((old_x + old_w > new_x + new_w) ? old_x + old_w : new_x + new_w);
        int16_t bottom = (int16_t)((old_y + old_h > new_y + new_h) ? old_y + old_h : new_y + new_h);
        ux = (int16_t)((old_x < new_x) ? old_x : new_x);
        uy = (int16_t)((old_y < new_y) ? old_y : new_y);
        uw = (int16_t)(right - ux);
        uh = (int16_t)(bottom - uy);
      }

      // Light-weight erase: solid ground color only
      game_lcd_fill_rect(ux, uy, uw, uh, FARM_GROUND_COLOR);
      // Restore the harvested tile background + crop if intersected
      if (anim_tile_idx >= 0) {
        uint8_t hrow = (uint8_t)(anim_tile_idx / FARM_GRID_COLS);
        uint8_t hcol = (uint8_t)(anim_tile_idx % FARM_GRID_COLS);
        int16_t htx = farm_tile_x(hcol);
        int16_t hty = farm_tile_y(hrow);
        if (!(htx + FARM_TILE_W <= ux || ux + uw <= htx || hty + FARM_TILE_H <= uy || uy + uh <= hty)) {
          draw_tile_bg_only(htx, hty, tiles[anim_tile_idx]);
        }
      }

      draw_player_in_farm(player_x, player_y, false);
      draw_harvest_anim(t_x, t_y, now - anim_start_ms);

      anim_prev_x = new_x;
      anim_prev_y = new_y;
      anim_prev_w = new_w;
      anim_prev_h = new_h;
      last_render_ms = now;
    }
  }

  if (reaction_mode != FARM_REACTION_NONE) {
    if (now - reaction_last_step_ms >= FARM_REACTION_STEP_MS) {
      reaction_prev_line_x = reaction_line_x;
      reaction_last_step_ms = now;
      reaction_line_x = (int16_t)(reaction_line_x + reaction_dir * FARM_REACTION_STEP_PX);
      if (reaction_line_x <= FARM_REACTION_BAR_X) {
        reaction_line_x = FARM_REACTION_BAR_X;
        reaction_dir = 1;
      }
      if (reaction_line_x >= FARM_REACTION_BAR_X + FARM_REACTION_BAR_W - FARM_REACTION_LINE_W) {
        reaction_line_x = (int16_t)(FARM_REACTION_BAR_X + FARM_REACTION_BAR_W - FARM_REACTION_LINE_W);
        reaction_dir = -1;
      }
      erase_reaction_line(reaction_mode, reaction_prev_line_x);
      draw_reaction_line(reaction_line_x);
      last_render_ms = now;
    }

    if (game_input_button_a_edge()) {
      bool success = reaction_hit_target(reaction_line_x);
      uint8_t target = reaction_tile_idx;

      if (success) {
        if (reaction_mode == FARM_REACTION_WATER) {
          game_buzzer_tone(900, 80);
          anim_mode = FARM_ANIM_WATERING;
          anim_tile_idx = target;
          anim_start_ms = now;
          anim_prev_w = 0;
          snprintf(status_line, sizeof(status_line), "WATERING...");
        } else {
          game_buzzer_tone(600, 80);
          tiles[target].state = FARM_TILE_FERTILIZED;
          tiles[target].state_since_ms = now;
          snprintf(status_line, sizeof(status_line), "FERTILIZED %u", (unsigned)(target + 1));
          state_changed = true;
          changed_tile_idx = (int8_t)target;
          sync_tiles_to_shared(tiles);
        }
      } else {
        game_buzzer_tone(200, 120);
        if (reaction_mode == FARM_REACTION_WATER) {
          snprintf(status_line, sizeof(status_line), "WATER FAILED RETRY");
        } else {
          snprintf(status_line, sizeof(status_line), "FERTILIZE FAIL RETRY");
        }
      }

      reaction_mode = FARM_REACTION_NONE;
      hud_dirty = true;
      clear_reaction_frame(tiles, selected_idx);
    }
  } else if (game_input_button_a_edge()) {
    if (selected_idx < 0) {
      snprintf(status_line, sizeof(status_line), "MOVE CLOSER TO PLOT");
      hud_dirty = true;
    } else {
      uint8_t selected = (uint8_t)selected_idx;
      switch (tiles[selected].state) {
        case FARM_TILE_EMPTY:
          game_buzzer_tone(400, 60);
          tiles[selected].state = FARM_TILE_SEEDED;
          snprintf(status_line, sizeof(status_line), "SOWED PLOT %u", (unsigned)(selected + 1));
          state_changed = true;
          changed_tile_idx = (int8_t)selected;
          sync_tiles_to_shared(tiles);
          break;
        case FARM_TILE_SEEDED:
          reaction_mode = FARM_REACTION_WATER;
          reaction_tile_idx = selected;
          reaction_line_x = FARM_REACTION_BAR_X;
          reaction_prev_line_x = reaction_line_x;
          reaction_dir = 1;
          reaction_last_step_ms = now;
          snprintf(status_line, sizeof(status_line), "STOP IN WATER ZONE");
          s_farm_need_full_redraw = true;
          break;
        case FARM_TILE_WATERED:
          reaction_mode = FARM_REACTION_FERTILIZE;
          reaction_tile_idx = selected;
          reaction_line_x = FARM_REACTION_BAR_X;
          reaction_prev_line_x = reaction_line_x;
          reaction_dir = 1;
          reaction_last_step_ms = now;
          snprintf(status_line, sizeof(status_line), "STOP IN FERTILIZE ZONE");
          s_farm_need_full_redraw = true;
          break;
        case FARM_TILE_FERTILIZED:
          snprintf(status_line, sizeof(status_line), "PLOT %u GROWING", (unsigned)(selected + 1));
          hud_dirty = true;
          break;
        case FARM_TILE_READY:
          anim_mode = FARM_ANIM_HARVESTING;
          anim_tile_idx = selected;
          anim_start_ms = now;
          anim_prev_w = 0;
          snprintf(status_line, sizeof(status_line), "HARVESTING...");
          s_farm_need_full_redraw = true;
          break;
        default:
          break;
      }
      hud_dirty = true;
    }
  }

  if (game_input_button_b_edge()) {
    if (reaction_mode != FARM_REACTION_NONE) {
      reaction_mode = FARM_REACTION_NONE;
      snprintf(status_line, sizeof(status_line), "REACTION CANCELED");
      clear_reaction_frame(tiles, selected_idx);
      hud_dirty = true;
    } else {
      anim_mode = FARM_ANIM_NONE;
      game_app_request_scene(GAME_SCENE_WORLD);
      return;
    }
  }

  // Harvest success melody update
  if (harvest_melody_idx != 0 || harvest_melody_ms != 0) {
    play_harvest_success_melody(&harvest_melody_idx, &harvest_melody_ms);
  }

  if (state_changed) {
    need_partial_render = true;
  }
  if (moved) {
    need_partial_render = true;
  }
  if (!s_farm_need_full_redraw && !need_partial_render &&
      (now - last_render_ms >= FARM_HUD_REFRESH_MS)) {
    hud_dirty = true;
  }

  if (can_render && s_farm_need_full_redraw) {
    uint8_t r;
    uint8_t c;

    game_lcd_fill_screen(FARM_GROUND_COLOR);
    game_lcd_draw_rect(0, 0, GAME_SCREEN_WIDTH, GAME_STATUS_BAR_HEIGHT, GAME_REF_COLOR_WHITE);
    game_lcd_draw_text(8, 7, "FARM", GAME_REF_COLOR_CYAN, GAME_REF_COLOR_BLACK, 1);

    game_lcd_draw_rect(0,
                       GAME_STATUS_BAR_HEIGHT,
                       GAME_SCREEN_WIDTH,
                       GAME_PLAY_AREA_HEIGHT,
                       GAME_REF_COLOR_WHITE);

    draw_fence();

    for (r = 0; r < 3; r++) {
      for (c = 0; c < 3; c++) {
        uint8_t tile_index = (uint8_t)(r * 3 + c);
        draw_tile_by_index(tile_index, tiles, selected_idx);
      }
    }

    draw_player_in_farm(player_x, player_y, false);

    shared = game_app_shared_resources();
    draw_farm_hud(selected_idx,
            tiles,
            status_line,
            (shared != 0) ? shared->food : 0u,
            (shared != 0) ? shared->metal : 0u);
    if (reaction_mode != FARM_REACTION_NONE) {
      draw_reaction_frame(reaction_mode);
      draw_reaction_line(reaction_line_x);
    }
    hud_dirty = false;

    s_farm_need_full_redraw = false;
    last_selected_idx = selected_idx;
    prev_player_x = player_x;
    prev_player_y = player_y;
    last_render_ms = now;
    return;
  }

  if (can_render && need_partial_render) {
    int16_t pw = GAME_REF_CHARACTER_RENDER_SIZE;
    int16_t ph = GAME_REF_CHARACTER_RENDER_SIZE;
    int16_t dx = player_x - prev_player_x;
    int16_t dy = player_y - prev_player_y;

    if (dx > 0) {
      redraw_farm_patch((int16_t)(prev_player_x - 1), (int16_t)(prev_player_y - 1),
                        (int16_t)(dx + 1), (int16_t)(ph + 1), tiles, selected_idx);
    } else if (dx < 0) {
      redraw_farm_patch((int16_t)(player_x + pw), (int16_t)(prev_player_y - 1),
                        (int16_t)(-dx + 1), (int16_t)(ph + 1), tiles, selected_idx);
    }

    {
      int16_t ox = (prev_player_x > player_x) ? prev_player_x : player_x;
      int16_t ow = (prev_player_x + pw < player_x + pw) ? prev_player_x + pw : player_x + pw;
      ow = (int16_t)(ow - ox);
      if (ow > 0) {
        if (dy > 0) {
          redraw_farm_patch(ox, (int16_t)(prev_player_y - 1), ow, (int16_t)(dy + 1),
                            tiles, selected_idx);
        } else if (dy < 0) {
          redraw_farm_patch(ox, (int16_t)(player_y + ph + 1), ow, (int16_t)(-dy + 1),
                            tiles, selected_idx);
        }
      }
    }

    // Erase old shadow (not covered by dy strips)
    redraw_farm_patch((int16_t)(prev_player_x + 4), (int16_t)(prev_player_y + ph - 2), 8, 3,
                      tiles, selected_idx);

    // Redraw tiles whose selection changed AFTER strip erase so they are not
    // overwritten by the ground-fill inside redraw_farm_patch.
    if (selected_idx != last_selected_idx) {
      if (last_selected_idx >= 0) {
        draw_tile_by_index((uint8_t)last_selected_idx, tiles, selected_idx);
      }
      if (selected_idx >= 0) {
        draw_tile_by_index((uint8_t)selected_idx, tiles, selected_idx);
      }
    }

    if (changed_tile_idx >= 0) {
      draw_tile_by_index((uint8_t)changed_tile_idx, tiles, selected_idx);
      changed_tile_idx = -1;
    }

    draw_player_in_farm(player_x, player_y, true);
  }

  if (can_render && (hud_dirty || state_changed || selected_idx != last_selected_idx)) {
    shared = game_app_shared_resources();
    draw_farm_hud(selected_idx,
                  tiles,
                  status_line,
                  (shared != 0) ? shared->food : 0u,
                  (shared != 0) ? shared->metal : 0u);
  }

  last_selected_idx = selected_idx;
  prev_player_x = player_x;
  prev_player_y = player_y;
  if (can_render && (s_farm_need_full_redraw || need_partial_render || hud_dirty || state_changed)) {
    last_render_ms = now;
  }
}

void game_farm_scene_request_full_redraw(void) {
  s_farm_force_full_redraw = true;
}
