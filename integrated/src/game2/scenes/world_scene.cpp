#include "game2/scenes/world_scene.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define COLOR_BG_GROUND 0x2104
#define COLOR_BG_DUST 0x39E7
#define COLOR_BG_SKY 0x5D5A
#define COLOR_BG_SKY_FADE 0x3C96
#define COLOR_BG_SKY_NIGHT 0x3C0C
#define COLOR_BG_SKY_FADE_NIGHT 0x1C06
#define COLOR_HILL_DARK 0x2A66
#define COLOR_HILL_LIGHT 0x438B
#define COLOR_HILL_DARK_NIGHT 0x1A44
#define COLOR_HILL_LIGHT_NIGHT 0x2A66
#define COLOR_FARM_SOIL 0x8A62
#define COLOR_FARM_WET 0x6241
#define COLOR_FARM_SEED 0x87E0
#define COLOR_FARM_FENCE 0xCE59
#define COLOR_FARM_STRAW 0xF6B0
#define COLOR_WATER_DEEP 0x033D
#define COLOR_WATER_LIGHT 0x04BF
#define COLOR_SHORE 0xA367
#define COLOR_REED 0x57E0
#define COLOR_FORGE_WALL 0x4A29
#define COLOR_FORGE_ROOF 0x18A3
#define COLOR_FORGE_FIRE 0xFD20
#define COLOR_FORGE_ANVIL 0x94B2
#define COLOR_METAL 0xCE79
#define COLOR_WOOD 0x8A62
#define COLOR_SHADOW 0x0841

#define WORLD_PLAY_BOTTOM 200

#define HOME_X 12
#define HOME_Y 150
#define HOME_W 58
#define HOME_H 48

#define FARM_X 12
#define FARM_Y 36
#define FARM_W 68
#define FARM_H 68

#define POND_CX 172
#define POND_CY 64
#define POND_R 28

// Full bounds of draw_pond_area (includes dock, reeds, stones, fish, POND text)
#define POND_AREA_X  134
#define POND_AREA_Y  34
#define POND_AREA_W  74
#define POND_AREA_H  82

#define FORGE_X 146
#define FORGE_Y 144
#define FORGE_W 80
#define FORGE_H 56

#include "game2/config/game_config.h"
#include "game2/config/reference/reference_style.h"
#include "game2/app/game_app.h"
#include "game2/drivers/lcd_driver.h"
#include "game2/drivers/platform_backend.h"
#include "game2/ui/character_renderer.h"

typedef enum {
  HOTSPOT_NONE = 0,
  HOTSPOT_HOME,
  HOTSPOT_FARM,
  HOTSPOT_POND,
  HOTSPOT_FORGE
} hotspot_t;

static bool s_world_force_full_redraw = false;

void game_world_scene_request_full_redraw(void) { s_world_force_full_redraw = true; }

static void draw_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
  int16_t x = r;
  int16_t y = 0;
  int16_t err = 0;

  while (x >= y) {
    game_lcd_draw_pixel(cx + x, cy + y, color);
    game_lcd_draw_pixel(cx + y, cy + x, color);
    game_lcd_draw_pixel(cx - y, cy + x, color);
    game_lcd_draw_pixel(cx - x, cy + y, color);
    game_lcd_draw_pixel(cx - x, cy - y, color);
    game_lcd_draw_pixel(cx - y, cy - x, color);
    game_lcd_draw_pixel(cx + y, cy - x, color);
    game_lcd_draw_pixel(cx + x, cy - y, color);

    y++;
    if (err <= 0) {
      err += 2 * y + 1;
    } else {
      x--;
      err += 2 * (y - x) + 1;
    }
  }
}

static void draw_filled_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
  int32_t rr = (int32_t)r * r;
  int16_t y;
  for (y = -r; y <= r; y++) {
    int16_t x = r;
    while (((int32_t)x * x + (int32_t)y * y) > rr && x > 0) {
      x--;
    }
    game_lcd_fill_rect((int16_t)(cx - x), (int16_t)(cy + y), (int16_t)(2 * x + 1), 1, color);
  }
}

static void draw_ground_pattern(void) {
  int16_t y;
  for (y = GAME_STATUS_BAR_HEIGHT + 3; y < WORLD_PLAY_BOTTOM; y += 8) {
    int16_t x;
    int16_t offset = (int16_t)(((y >> 3) & 1) ? 3 : 0);
    for (x = (int16_t)(4 + offset); x < GAME_SCREEN_WIDTH - 6; x += 16) {
      game_lcd_fill_rect(x, y, 3, 1, COLOR_BG_DUST);
    }
  }
}

static void draw_tree(int16_t x, int16_t y) {
  game_lcd_fill_rect((int16_t)(x + 4), (int16_t)(y + 10), 4, 8, COLOR_WOOD);
  game_lcd_fill_rect(x, y + 2, 12, 10, COLOR_HILL_LIGHT);
  game_lcd_fill_rect((int16_t)(x + 2), y, 8, 4, COLOR_HILL_DARK);
}

static void draw_boulder(int16_t x, int16_t y) {
  game_lcd_fill_rect(x, y + 2, 10, 6, 0x7BEF);
  game_lcd_fill_rect((int16_t)(x + 2), y, 6, 3, 0x9CD3);
}

static void draw_world_background(void) {
  int16_t y;

  game_shared_resources_t* shared = game_app_shared_resources();
  bool is_night = (shared != 0) && ((shared->day % 2) == 0);

  uint16_t sky_top    = is_night ? COLOR_BG_SKY_NIGHT    : COLOR_BG_SKY;
  uint16_t sky_fade   = is_night ? COLOR_BG_SKY_FADE_NIGHT : COLOR_BG_SKY_FADE;
  uint16_t hill_dark  = is_night ? COLOR_HILL_DARK_NIGHT : COLOR_HILL_DARK;
  uint16_t hill_light = is_night ? COLOR_HILL_LIGHT_NIGHT : COLOR_HILL_LIGHT;

  for (y = GAME_STATUS_BAR_HEIGHT; y < GAME_STATUS_BAR_HEIGHT + 18; y++) {
    uint16_t c = (y < GAME_STATUS_BAR_HEIGHT + 9) ? sky_top : sky_fade;
    game_lcd_fill_rect(1, y, GAME_SCREEN_WIDTH - 2, 1, c);
  }

  game_lcd_fill_rect(0, 38, 80, 12, hill_dark);
  game_lcd_fill_rect(64, 34, 70, 16, hill_light);
  game_lcd_fill_rect(128, 36, 62, 14, hill_dark);
  game_lcd_fill_rect(182, 32, 58, 18, hill_light);

  draw_tree(8, 58);
  draw_tree(98, 52);
  draw_tree(210, 64);
  draw_boulder(98, 154);
  draw_boulder(212, 152);
}

static void draw_farm_area(void) {
  int16_t base_x = FARM_X;
  int16_t base_y = FARM_Y;
  int16_t row;
  int16_t col;

  game_lcd_fill_rect(base_x, base_y, FARM_W, FARM_H, 0x4208);
  game_lcd_draw_rect(base_x, base_y, FARM_W, FARM_H, 0xA514);

  for (row = 0; row < 3; row++) {
    for (col = 0; col < 3; col++) {
      int16_t tx = (int16_t)(base_x + 5 + col * 20);
      int16_t ty = (int16_t)(base_y + 6 + row * 20);
      uint16_t tile_color = (((row + col) & 1) == 0) ? COLOR_FARM_SOIL : COLOR_FARM_WET;
      game_lcd_fill_rect(tx, ty, 18, 18, tile_color);
      game_lcd_draw_rect(tx, ty, 18, 18, 0x39C7);

      game_lcd_draw_pixel((int16_t)(tx + 8), (int16_t)(ty + 8), COLOR_FARM_SEED);
      game_lcd_draw_pixel((int16_t)(tx + 7), (int16_t)(ty + 9), COLOR_FARM_SEED);
      game_lcd_draw_pixel((int16_t)(tx + 9), (int16_t)(ty + 9), COLOR_FARM_SEED);

      game_lcd_fill_rect((int16_t)(tx + 2), (int16_t)(ty + 13), 14, 1, 0x51E8);
    }
  }

  game_lcd_fill_rect(base_x + 2, base_y + 2, 2, FARM_H - 4, COLOR_FARM_FENCE);
  game_lcd_fill_rect(base_x + FARM_W - 4, base_y + 2, 2, FARM_H - 4, COLOR_FARM_FENCE);
  game_lcd_fill_rect(base_x + 2, base_y + 2, FARM_W - 4, 2, COLOR_FARM_FENCE);
  game_lcd_fill_rect(base_x + 2, base_y + FARM_H - 4, FARM_W - 4, 2, COLOR_FARM_FENCE);

  game_lcd_fill_rect(base_x + FARM_W - 20, base_y - 8, 16, 8, 0xA514);
  game_lcd_fill_rect(base_x + FARM_W - 18, base_y - 6, 5, 6, 0xB596);
  game_lcd_fill_rect(base_x + FARM_W - 10, base_y - 6, 6, 4, COLOR_FARM_STRAW);

  game_lcd_draw_text(base_x + 14, base_y + FARM_H + 4, "FARM", GAME_REF_COLOR_YELLOW, COLOR_BG_GROUND, 1);
}

static void draw_pond_area(void) {
  int16_t cx = POND_CX;
  int16_t cy = POND_CY;

  draw_filled_circle(cx, cy, POND_R, COLOR_WATER_DEEP);
  draw_filled_circle(cx + 4, cy - 2, 12, 0x04BF);
  draw_filled_circle(cx - 6, cy + 4, 10, 0x03F8);
  draw_circle(cx, cy, POND_R, GAME_REF_COLOR_CYAN);

  // Deep blue ripples
  draw_circle(cx + 2, cy + 2, 16, 0x033D);
  draw_circle(cx - 3, cy - 1, 10, 0x033D);
  draw_circle(cx + 5, cy - 5, 6, 0x033D);

  game_lcd_fill_rect(137, 73, 2, 8, COLOR_REED);
  game_lcd_fill_rect(140, 70, 2, 10, COLOR_REED);
  game_lcd_fill_rect(193, 64, 2, 9, COLOR_REED);
  game_lcd_fill_rect(196, 68, 2, 7, COLOR_REED);
  draw_boulder(136, 94);
  draw_boulder(192, 90);

  game_lcd_fill_rect(145, 88, 24, 8, 0x8A62);
  game_lcd_draw_rect(145, 88, 24, 8, 0xB596);
  game_lcd_fill_rect(152, 96, 4, 12, 0x8A62);
  game_lcd_fill_rect(160, 96, 4, 12, 0x8A62);

  // Realistic grey fish 1
  game_lcd_fill_rect((int16_t)(cx - 8), (int16_t)(cy + 4), 10, 4, 0x8A62);
  game_lcd_fill_rect((int16_t)(cx - 10), (int16_t)(cy + 3), 3, 6, 0x8A62);
  game_lcd_fill_rect((int16_t)(cx - 5), (int16_t)(cy + 2), 3, 2, 0x9CD3);
  game_lcd_fill_rect((int16_t)(cx + 0), (int16_t)(cy + 4), 2, 2, 0xFFFF);

  // Realistic grey fish 2
  game_lcd_fill_rect((int16_t)(cx + 4), (int16_t)(cy - 6), 8, 3, 0x8A62);
  game_lcd_fill_rect((int16_t)(cx + 2), (int16_t)(cy - 7), 3, 5, 0x8A62);
  game_lcd_fill_rect((int16_t)(cx + 6), (int16_t)(cy - 8), 2, 2, 0x9CD3);
  game_lcd_fill_rect((int16_t)(cx + 10), (int16_t)(cy - 6), 2, 2, 0xFFFF);

  game_lcd_draw_text(150, 102, "POND", GAME_REF_COLOR_YELLOW, COLOR_BG_GROUND, 1);
}

static void draw_forge_area(void) {
  int16_t bx = FORGE_X;
  int16_t by = FORGE_Y;

  game_lcd_fill_rect(bx, by + 10, FORGE_W, FORGE_H, COLOR_FORGE_WALL);
  game_lcd_draw_rect(bx, by + 10, FORGE_W, FORGE_H, 0x9CF3);

  game_lcd_fill_rect(bx + 2, by + 24, FORGE_W - 4, 1, 0x632C);
  game_lcd_fill_rect(bx + 2, by + 38, FORGE_W - 4, 1, 0x632C);
  game_lcd_fill_rect(bx + 2, by + 52, FORGE_W - 4, 1, 0x632C);

  game_lcd_fill_rect(bx - 4, by, FORGE_W + 8, 14, COLOR_FORGE_ROOF);
  game_lcd_draw_rect(bx - 4, by, FORGE_W + 8, 14, 0x7BEF);

  game_lcd_fill_rect(bx + 8, by + 30, 24, 22, 0x18A3);
  game_lcd_fill_rect(bx + 14, by + 36, 12, 10, COLOR_FORGE_FIRE);
  game_lcd_draw_rect(bx + 14, by + 36, 12, 10, 0xFFE0);

  game_lcd_fill_rect(bx + 50, by + 44, 26, 8, COLOR_FORGE_ANVIL);
  game_lcd_fill_rect(bx + 58, by + 52, 10, 10, 0x5AEB);
  game_lcd_draw_rect(bx + 50, by + 44, 26, 8, 0xC618);

  game_lcd_fill_rect(bx + 38, by + 22, 34, 3, COLOR_WOOD);
  game_lcd_fill_rect(bx + 42, by + 25, 2, 9, COLOR_METAL);
  game_lcd_fill_rect(bx + 50, by + 25, 2, 8, COLOR_METAL);
  game_lcd_fill_rect(bx + 58, by + 25, 2, 7, COLOR_METAL);
  game_lcd_fill_rect(bx + 66, by + 25, 2, 8, COLOR_METAL);

  game_lcd_fill_rect(bx + 72, by + 14, 10, 24, 0x39C7);
  game_lcd_fill_rect(bx + 74, by + 10, 6, 4, 0x7BEF);
  game_lcd_fill_rect(bx + 76, by + 6, 2, 3, 0x9CD3);

  game_lcd_fill_rect(bx + 8, by + FORGE_H + 2, 14, 8, COLOR_WOOD);
  game_lcd_draw_rect(bx + 8, by + FORGE_H + 2, 14, 8, 0xB596);
  game_lcd_fill_rect(bx + 26, by + FORGE_H + 2, 10, 8, COLOR_WOOD);
  game_lcd_draw_rect(bx + 26, by + FORGE_H + 2, 10, 8, 0xB596);

  game_lcd_draw_text(152, 188, "FORGE", GAME_REF_COLOR_YELLOW, COLOR_BG_GROUND, 1);
}

static void draw_home_area(void) {
  int16_t bx = HOME_X;
  int16_t by = HOME_Y;

  // Front steps
  game_lcd_fill_rect(bx + 18, by + HOME_H - 2, 22, 4, 0x9CD3);
  game_lcd_fill_rect(bx + 16, by + HOME_H, 26, 3, 0x7BEF);

  // House body
  game_lcd_fill_rect(bx + 4, by + 8, 50, 42, 0xCE59);
  game_lcd_draw_rect(bx + 4, by + 8, 50, 42, 0x8A62);

  // Brick texture lines
  game_lcd_fill_rect(bx + 6, by + 16, 46, 1, 0xB596);
  game_lcd_fill_rect(bx + 6, by + 24, 46, 1, 0xB596);
  game_lcd_fill_rect(bx + 6, by + 32, 46, 1, 0xB596);
  game_lcd_fill_rect(bx + 6, by + 40, 46, 1, 0xB596);

  // Pointed roof (triangular look with highlight)
  game_lcd_fill_rect(bx, by, 58, 10, 0xB000);
  game_lcd_fill_rect(bx + 2, by + 2, 54, 6, 0xD000);
  game_lcd_draw_rect(bx, by, 58, 10, 0x7BEF);

  // Door with handle
  game_lcd_fill_rect(bx + 22, by + 28, 12, 22, 0x8A62);
  game_lcd_draw_rect(bx + 22, by + 28, 12, 22, 0x6241);
  game_lcd_fill_rect(bx + 31, by + 38, 2, 3, 0xFFE0);

  // Left window with cross frame
  game_lcd_fill_rect(bx + 8, by + 18, 10, 10, 0x04BF);
  game_lcd_draw_rect(bx + 8, by + 18, 10, 10, 0xFFFF);
  game_lcd_fill_rect(bx + 12, by + 18, 2, 10, 0xFFFF);
  game_lcd_fill_rect(bx + 8, by + 22, 10, 2, 0xFFFF);

  // Right window with cross frame
  game_lcd_fill_rect(bx + 36, by + 18, 10, 10, 0x04BF);
  game_lcd_draw_rect(bx + 36, by + 18, 10, 10, 0xFFFF);
  game_lcd_fill_rect(bx + 40, by + 18, 2, 10, 0xFFFF);
  game_lcd_fill_rect(bx + 36, by + 22, 10, 2, 0xFFFF);

  // Chimney
  game_lcd_fill_rect(bx + 40, by - 4, 8, 10, 0x39C7);
  game_lcd_fill_rect(bx + 42, by - 6, 4, 3, 0x7BEF);

  // Smoke puffs
  game_lcd_fill_rect(bx + 43, by - 10, 3, 3, 0x9CD3);
  game_lcd_fill_rect(bx + 45, by - 14, 2, 2, 0xBDF7);
  game_lcd_fill_rect(bx + 42, by - 16, 2, 2, 0xBDF7);

  // Porch columns
  game_lcd_fill_rect(bx + 18, by + 26, 3, 24, 0xFFFF);
  game_lcd_fill_rect(bx + 37, by + 26, 3, 24, 0xFFFF);

  // Small garden bushes
  game_lcd_fill_rect(bx + 2, by + HOME_H + 2, 8, 6, 0x07E0);
  game_lcd_fill_rect(bx + HOME_W - 10, by + HOME_H + 2, 8, 6, 0x07E0);
  game_lcd_fill_rect(bx + 4, by + HOME_H + 4, 2, 2, 0x87E0);
  game_lcd_fill_rect(bx + HOME_W - 6, by + HOME_H + 4, 2, 2, 0x87E0);

  game_lcd_draw_text(bx + 10, by + HOME_H + 10, "HOME", GAME_REF_COLOR_YELLOW, GAME_REF_COLOR_BLACK, 1);
}

static hotspot_t detect_hotspot(int16_t px, int16_t py) {
  int16_t player_cx = (int16_t)(px + GAME_REF_CHARACTER_RENDER_SIZE / 2);
  int16_t player_cy = (int16_t)(py + GAME_REF_CHARACTER_RENDER_SIZE / 2);
  int16_t dx = (int16_t)(player_cx - POND_CX);
  int16_t dy = (int16_t)(player_cy - POND_CY);

  if (player_cx >= HOME_X - 18 && player_cx <= HOME_X + HOME_W + 18 &&
      player_cy >= HOME_Y - 14 && player_cy <= HOME_Y + HOME_H + 14) {
    return HOTSPOT_HOME;
  }

  if (player_cx >= FARM_X - 18 && player_cx <= FARM_X + FARM_W + 18 &&
      player_cy >= FARM_Y - 14 && player_cy <= FARM_Y + FARM_H + 14) {
    return HOTSPOT_FARM;
  }

  if ((dx * dx + dy * dy) <= (44 * 44)) {
    return HOTSPOT_POND;
  }

  if (player_cx >= FORGE_X - 18 && player_cx <= FORGE_X + FORGE_W + 18 &&
      player_cy >= FORGE_Y - 14 && player_cy <= FORGE_Y + FORGE_H + 18) {
    return HOTSPOT_FORGE;
  }

  return HOTSPOT_NONE;
}

static const char* hotspot_prompt(hotspot_t hotspot) {
  switch (hotspot) {
    case HOTSPOT_HOME:
      return "A:ENTER HOME";
    case HOTSPOT_FARM:
      return "A:ENTER FARM";
    case HOTSPOT_POND:
      return "A:ENTER POND";
    case HOTSPOT_FORGE:
      return "A:ENTER FORGE";
    default:
      return "";
  }
}

static bool rect_intersects(int16_t x, int16_t y, int16_t w, int16_t h,
                            int16_t rx, int16_t ry, int16_t rw, int16_t rh) {
  return !(x + w <= rx || rx + rw <= x || y + h <= ry || ry + rh <= y);
}

static void draw_player_with_shadow(int16_t player_x, int16_t player_y, bool moving) {
  int16_t shadow_w = moving ? 6 : 8;
  int16_t shadow_h = moving ? 2 : 3;
  int16_t shadow_x = (int16_t)(player_x + (moving ? 5 : 4));
  int16_t shadow_y = (int16_t)(player_y + GAME_REF_CHARACTER_RENDER_SIZE - (moving ? 1 : 2));

  game_lcd_fill_rect(shadow_x, shadow_y, shadow_w, shadow_h, COLOR_SHADOW);
  game_character_renderer_draw(player_x, player_y);
}

static bool is_in_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                       int16_t rx, int16_t ry, int16_t rw, int16_t rh) {
  return !(x + w <= rx || rx + rw <= x || y + h <= ry || ry + rh <= y);
}

static bool is_walk_blocked(int16_t player_x, int16_t player_y) {
  int16_t w = GAME_REF_CHARACTER_RENDER_SIZE;
  int16_t h = GAME_REF_CHARACTER_RENDER_SIZE;
  int16_t margin = 4;

  if (is_in_rect(player_x, player_y, w, h,
                 HOME_X - margin, HOME_Y - margin, HOME_W + margin * 2, HOME_H + margin * 2)) return true;

  if (is_in_rect(player_x, player_y, w, h,
                 FARM_X - margin, FARM_Y - margin, FARM_W + margin * 2, FARM_H + margin * 2)) return true;

  if (is_in_rect(player_x, player_y, w, h,
                 POND_CX - POND_R - margin, POND_CY - POND_R - margin,
                 POND_R * 2 + margin * 2, POND_R * 2 + margin * 2)) return true;

  if (is_in_rect(player_x, player_y, w, h,
                 FORGE_X - margin, FORGE_Y - margin, FORGE_W + margin * 2, FORGE_H + margin * 2)) return true;

  return false;
}

static void draw_action_bar(hotspot_t hotspot, bool enter_edge) {
  char line[24];

  game_lcd_fill_rect(0, 200, GAME_SCREEN_WIDTH, 40, 0x2104);
  game_lcd_draw_rect(0, 200, GAME_SCREEN_WIDTH, 40, GAME_REF_COLOR_WHITE);
  if (hotspot != HOTSPOT_NONE) {
    game_lcd_draw_text(10, 212, hotspot_prompt(hotspot), GAME_REF_COLOR_YELLOW, 0x2104, 1);
    if (enter_edge) {
      switch (hotspot) {
        case HOTSPOT_HOME:
          snprintf(line, sizeof(line), "ENTER HOME");
          break;
        case HOTSPOT_FARM:
          snprintf(line, sizeof(line), "ENTER FARM");
          break;
        case HOTSPOT_POND:
          snprintf(line, sizeof(line), "ENTER POND");
          break;
        case HOTSPOT_FORGE:
          snprintf(line, sizeof(line), "ENTER FORGE");
          break;
        default:
          line[0] = '\0';
          break;
      }
      game_lcd_draw_text(124, 212, line, GAME_REF_COLOR_CYAN, 0x2104, 1);
    }
  } else {
    game_lcd_draw_text(10, 212, "MOVE WITH JOYSTICK", GAME_REF_COLOR_WHITE, 0x2104, 1);
  }
}

static void redraw_ground_patch(int16_t x, int16_t y, int16_t w, int16_t h) {
  int16_t sx = x;
  int16_t sy = y;
  int16_t ex = (int16_t)(x + w - 1);
  int16_t ey = (int16_t)(y + h - 1);

  if (sx < 1) sx = 1;
  if (sy < GAME_STATUS_BAR_HEIGHT + 1) sy = GAME_STATUS_BAR_HEIGHT + 1;
  if (ex > GAME_SCREEN_WIDTH - 2) ex = GAME_SCREEN_WIDTH - 2;
  if (ey > WORLD_PLAY_BOTTOM - 1) ey = WORLD_PLAY_BOTTOM - 1;
  if (sx > ex || sy > ey) return;

  game_lcd_fill_rect(sx, sy, (int16_t)(ex - sx + 1), (int16_t)(ey - sy + 1), COLOR_BG_GROUND);

  {
    int16_t py;
    for (py = GAME_STATUS_BAR_HEIGHT + 3; py < WORLD_PLAY_BOTTOM; py += 8) {
      if (py < sy || py > ey) continue;
      {
        int16_t px;
        int16_t offset = (int16_t)(((py >> 3) & 1) ? 3 : 0);
        for (px = (int16_t)(4 + offset); px < GAME_SCREEN_WIDTH - 6; px += 16) {
          if (px >= sx && (px + 2) <= ex) {
            game_lcd_fill_rect(px, py, 3, 1, COLOR_BG_DUST);
          }
        }
      }
    }
  }

  if (rect_intersects(sx, sy, (int16_t)(ex - sx + 1), (int16_t)(ey - sy + 1),
                      HOME_X, HOME_Y, HOME_W, HOME_H)) {
    draw_home_area();
  }

  if (rect_intersects(sx, sy, (int16_t)(ex - sx + 1), (int16_t)(ey - sy + 1),
                      FARM_X, FARM_Y - 10, FARM_W, FARM_H + 18)) {
    draw_farm_area();
  }

  if (rect_intersects(sx, sy, (int16_t)(ex - sx + 1), (int16_t)(ey - sy + 1),
                      POND_AREA_X, POND_AREA_Y, POND_AREA_W, POND_AREA_H)) {
    draw_pond_area();
  }

  if (rect_intersects(sx, sy, (int16_t)(ex - sx + 1), (int16_t)(ey - sy + 1),
                      FORGE_X, FORGE_Y, FORGE_W, FORGE_H + 18)) {
    draw_forge_area();
  }

  if (rect_intersects(sx, sy, (int16_t)(ex - sx + 1), (int16_t)(ey - sy + 1), 8, 58, 12, 18)) {
    draw_tree(8, 58);
  }
  if (rect_intersects(sx, sy, (int16_t)(ex - sx + 1), (int16_t)(ey - sy + 1), 98, 52, 12, 18)) {
    draw_tree(98, 52);
  }
  if (rect_intersects(sx, sy, (int16_t)(ex - sx + 1), (int16_t)(ey - sy + 1), 210, 64, 12, 18)) {
    draw_tree(210, 64);
  }
  if (rect_intersects(sx, sy, (int16_t)(ex - sx + 1), (int16_t)(ey - sy + 1), 98, 154, 10, 9)) {
    draw_boulder(98, 154);
  }
  if (rect_intersects(sx, sy, (int16_t)(ex - sx + 1), (int16_t)(ey - sy + 1), 212, 152, 10, 9)) {
    draw_boulder(212, 152);
  }
}

static void render_world(int16_t player_x, int16_t player_y, hotspot_t hotspot, bool enter_edge) {
  game_lcd_fill_screen(COLOR_BG_GROUND);

  game_lcd_draw_rect(0, 0, GAME_SCREEN_WIDTH, GAME_STATUS_BAR_HEIGHT, GAME_REF_COLOR_WHITE);
  game_lcd_draw_text(8, 7, "WORLD MAP", GAME_REF_COLOR_CYAN, GAME_REF_COLOR_BLACK, 1);

  game_lcd_draw_rect(0,
                     GAME_STATUS_BAR_HEIGHT,
                     GAME_SCREEN_WIDTH,
                     GAME_PLAY_AREA_HEIGHT,
                     GAME_REF_COLOR_WHITE);

  draw_world_background();
  draw_ground_pattern();

  draw_home_area();
  draw_farm_area();
  draw_pond_area();
  draw_forge_area();

  draw_player_with_shadow(player_x, player_y, false);
  draw_action_bar(hotspot, enter_edge);
}

void game_world_scene_update(void) {
  static bool initialized = false;
  static int16_t player_x = 104;
  static int16_t player_y = 112;
  static int16_t prev_player_x = 104;
  static int16_t prev_player_y = 112;
  static uint32_t last_move_ms = 0;
  static uint32_t last_render_ms = 0;
  static hotspot_t last_hotspot = HOTSPOT_NONE;
  hotspot_t hotspot;
  bool moved = false;
  bool enter_edge = false;
  bool need_full_redraw = false;
  bool need_partial_redraw = false;
  bool action_bar_dirty = false;

  if (s_world_force_full_redraw) {
    initialized = false;
    s_world_force_full_redraw = false;
  }

  if (!initialized) {
    game_lcd_init();
    game_character_renderer_init();
    render_world(player_x, player_y, HOTSPOT_NONE, false);
    initialized = true;
  }

  game_input_update();

  if (game_platform_millis() - last_move_ms >= 14u) {
    last_move_ms = game_platform_millis();
    {
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
        int16_t candidate_x = next_x;
        int16_t candidate_y = player_y;
        if (!is_walk_blocked(candidate_x, candidate_y)) {
          player_x = candidate_x;
          moved = true;
        }

        candidate_x = player_x;
        candidate_y = next_y;
        if (!is_walk_blocked(candidate_x, candidate_y)) {
          player_y = candidate_y;
          moved = true;
        }
      }
    }
  }

  if (player_x < GAME_PLAY_AREA_X) player_x = GAME_PLAY_AREA_X;
  if (player_y < GAME_STATUS_BAR_HEIGHT + 2) player_y = GAME_STATUS_BAR_HEIGHT + 2;
  if (player_x > GAME_SCREEN_WIDTH - GAME_REF_CHARACTER_RENDER_SIZE - 2)
    player_x = GAME_SCREEN_WIDTH - GAME_REF_CHARACTER_RENDER_SIZE - 2;
  if (player_y > 200 - GAME_REF_CHARACTER_RENDER_SIZE - 2)
    player_y = 200 - GAME_REF_CHARACTER_RENDER_SIZE - 2;

  hotspot = detect_hotspot(player_x, player_y);
  if (hotspot != HOTSPOT_NONE && game_input_button_a_edge()) {
    enter_edge = true;

    switch (hotspot) {
      case HOTSPOT_HOME:
        game_app_request_scene(GAME_SCENE_HOME);
        break;
      case HOTSPOT_FARM:
        game_app_request_scene(GAME_SCENE_FARM);
        break;
      case HOTSPOT_POND:
        game_app_request_scene(GAME_SCENE_FISHING);
        break;
      case HOTSPOT_FORGE:
        game_app_request_scene(GAME_SCENE_FORGE);
        break;
      default:
        break;
    }
  }

  if (enter_edge) {
    need_full_redraw = true;
    action_bar_dirty = true;
  } else if (hotspot != last_hotspot) {
    action_bar_dirty = true;
  }

  if (moved && !need_full_redraw) {
    need_partial_redraw = true;
  }

  if (!need_full_redraw && !need_partial_redraw && action_bar_dirty) {
    draw_action_bar(hotspot, false);
    last_render_ms = game_platform_millis();
  }

  if (need_full_redraw) {
    render_world(player_x, player_y, hotspot, enter_edge);
    last_render_ms = game_platform_millis();
  } else if (need_partial_redraw) {
    int16_t pw = GAME_REF_CHARACTER_RENDER_SIZE;
    int16_t ph = GAME_REF_CHARACTER_RENDER_SIZE;
    int16_t dx = player_x - prev_player_x;
    int16_t dy = player_y - prev_player_y;

    if (dx > 0) {
      redraw_ground_patch((int16_t)(prev_player_x - 1), (int16_t)(prev_player_y - 1),
                          (int16_t)(dx + 1), (int16_t)(ph + 4));
    } else if (dx < 0) {
      redraw_ground_patch((int16_t)(player_x + pw), (int16_t)(prev_player_y - 1),
                          (int16_t)(-dx + 1), (int16_t)(ph + 4));
    }

    {
      int16_t ox = (prev_player_x > player_x) ? prev_player_x : player_x;
      int16_t ow = (prev_player_x + pw < player_x + pw) ? prev_player_x + pw : player_x + pw;
      ow = (int16_t)(ow - ox);
      if (ow > 0) {
        if (dy > 0) {
          redraw_ground_patch(ox, (int16_t)(prev_player_y - 1), ow, (int16_t)(dy + 1));
        } else if (dy < 0) {
          redraw_ground_patch(ox, (int16_t)(player_y + ph + 1), ow, (int16_t)(-dy + 1));
        }
      }
    }

    draw_player_with_shadow(player_x, player_y, true);
    if (action_bar_dirty) {
      draw_action_bar(hotspot, false);
    }
    last_render_ms = game_platform_millis();
  }

  prev_player_x = player_x;
  prev_player_y = player_y;
  last_hotspot = hotspot;
}
