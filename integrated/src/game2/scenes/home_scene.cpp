#include "game2/scenes/home_scene.h"

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

#define HOME_PANEL_COLOR   0x2104
#define HOME_FLOOR_LIGHT   0x8A62
#define HOME_FLOOR_DARK    0x6241
#define HOME_WALL_COLOR    0xB596
#define HOME_WALL_DARK     0x8A62
#define HOME_WINDOW_SKY    0x04BF
#define HOME_WINDOW_FRAME  0xFFFF
#define HOME_BED_FRAME     0x8A62
#define HOME_BED_MATTRESS  0xCE59
#define HOME_BED_PILLOW    0xFFFF
#define HOME_BED_BLANKET   0xB000
#define HOME_ARMOR_STAND   0x6241
#define HOME_ARMOR_METAL   0xAD55
#define HOME_ARMOR_SUIT    0x5326
#define HOME_ARMOR_VISOR   0x07FF

#define PLAYER_MAX_HP 100

#define PLAYER_W GAME_REF_CHARACTER_RENDER_SIZE
#define PLAYER_H GAME_REF_CHARACTER_RENDER_SIZE

#define BED_X   26
#define BED_Y   140
#define BED_W   52
#define BED_H   42

#define ARMOR_X 158
#define ARMOR_Y 96
#define ARMOR_W 32
#define ARMOR_H 64

#define WINDOW_X 96
#define WINDOW_Y 36
#define WINDOW_W 44
#define WINDOW_H 32

static bool s_home_force_full_redraw = false;

static void draw_floor(void) {
  int16_t y;
  for (y = GAME_STATUS_BAR_HEIGHT; y < 200; y += 12) {
    game_lcd_fill_rect(0, y, GAME_SCREEN_WIDTH, 6, HOME_FLOOR_LIGHT);
    game_lcd_fill_rect(0, y + 6, GAME_SCREEN_WIDTH, 6, HOME_FLOOR_DARK);
  }
}

static void draw_walls(void) {
  game_lcd_fill_rect(0, GAME_STATUS_BAR_HEIGHT, GAME_SCREEN_WIDTH, 6, HOME_WALL_COLOR);
  game_lcd_fill_rect(0, 194, GAME_SCREEN_WIDTH, 6, HOME_WALL_DARK);
  game_lcd_fill_rect(0, GAME_STATUS_BAR_HEIGHT, 6, 200 - GAME_STATUS_BAR_HEIGHT, HOME_WALL_COLOR);
  game_lcd_fill_rect(GAME_SCREEN_WIDTH - 6, GAME_STATUS_BAR_HEIGHT, 6, 200 - GAME_STATUS_BAR_HEIGHT, HOME_WALL_COLOR);
}

static void draw_window(void) {
  int16_t x = WINDOW_X;
  int16_t y = WINDOW_Y;

  game_lcd_fill_rect(x, y, WINDOW_W, WINDOW_H, HOME_WINDOW_FRAME);
  game_lcd_fill_rect(x + 2, y + 2, WINDOW_W - 4, WINDOW_H - 4, HOME_WINDOW_SKY);
  game_lcd_fill_rect(x + WINDOW_W / 2 - 1, y + 2, 2, WINDOW_H - 4, HOME_WINDOW_FRAME);
  game_lcd_fill_rect(x + 2, y + WINDOW_H / 2 - 1, WINDOW_W - 4, 2, HOME_WINDOW_FRAME);

  // Window sill
  game_lcd_fill_rect(x - 4, y + WINDOW_H, WINDOW_W + 8, 4, HOME_WALL_DARK);
}

static void draw_bed(void) {
  int16_t x = BED_X;
  int16_t y = BED_Y;

  game_lcd_fill_rect(x, y, BED_W, BED_H, HOME_BED_FRAME);
  game_lcd_draw_rect(x, y, BED_W, BED_H, 0x6241);

  game_lcd_fill_rect(x + 3, y + 3, BED_W - 6, BED_H - 6, HOME_BED_MATTRESS);

  game_lcd_fill_rect(x + 6, y + 6, 18, 10, HOME_BED_PILLOW);
  game_lcd_draw_rect(x + 6, y + 6, 18, 10, 0x9CD3);

  game_lcd_fill_rect(x + 3, y + 18, BED_W - 6, BED_H - 21, HOME_BED_BLANKET);
  game_lcd_draw_rect(x + 3, y + 18, BED_W - 6, BED_H - 21, 0x9000);

  game_lcd_fill_rect(x + 8, y + 22, BED_W - 16, 1, 0x9000);
  game_lcd_fill_rect(x + 8, y + 28, BED_W - 16, 1, 0x9000);
  game_lcd_fill_rect(x + 8, y + 34, BED_W - 16, 1, 0x9000);
}

static void draw_armor_stand(void) {
  int16_t bx = ARMOR_X + 12;
  int16_t by = ARMOR_Y;

  game_lcd_fill_rect(bx - 4, by + 56, 16, 4, HOME_ARMOR_STAND);
  game_lcd_fill_rect(bx + 2, by + 10, 4, 48, HOME_ARMOR_STAND);
  game_lcd_fill_rect(bx - 8, by + 14, 24, 3, HOME_ARMOR_STAND);

  game_lcd_fill_rect(bx - 6, by, 20, 12, HOME_ARMOR_METAL);
  game_lcd_draw_rect(bx - 6, by, 20, 12, 0x7BEF);
  game_lcd_fill_rect(bx + 2, by + 5, 4, 3, HOME_ARMOR_VISOR);

  game_lcd_fill_rect(bx - 4, by + 16, 16, 20, HOME_ARMOR_METAL);
  game_lcd_draw_rect(bx - 4, by + 16, 16, 20, 0x7BEF);
  game_lcd_fill_rect(bx, by + 20, 8, 2, 0x7BEF);
  game_lcd_fill_rect(bx, by + 26, 8, 2, 0x7BEF);
  game_lcd_fill_rect(bx + 2, by + 22, 4, 6, HOME_ARMOR_VISOR);

  game_lcd_fill_rect(bx - 2, by + 38, 6, 18, HOME_ARMOR_SUIT);
  game_lcd_fill_rect(bx + 6, by + 38, 6, 18, HOME_ARMOR_SUIT);
  game_lcd_draw_rect(bx - 2, by + 38, 6, 18, 0x31A6);
  game_lcd_draw_rect(bx + 6, by + 38, 6, 18, 0x31A6);
}

static bool is_near_bed(int16_t px, int16_t py) {
  int16_t cx = (int16_t)(px + PLAYER_W / 2);
  int16_t cy = (int16_t)(py + PLAYER_H / 2);
  int16_t bx = (int16_t)(BED_X + BED_W / 2);
  int16_t by = (int16_t)(BED_Y + BED_H / 2);
  int16_t dx = (int16_t)(cx - bx);
  int16_t dy = (int16_t)(cy - by);
  return (dx * dx + dy * dy) <= (40 * 40);
}

static void draw_home_hud(bool near_bed) {
  game_lcd_fill_rect(0, 200, GAME_SCREEN_WIDTH, 40, HOME_PANEL_COLOR);
  game_lcd_draw_rect(0, 200, GAME_SCREEN_WIDTH, 40, GAME_REF_COLOR_WHITE);
  if (near_bed) {
    game_lcd_draw_text(8, 206, "A:SLEEP", GAME_REF_COLOR_YELLOW, HOME_PANEL_COLOR, 1);
  } else {
    game_lcd_draw_text(8, 206, "B:RETURN TO WORLD", GAME_REF_COLOR_CYAN, HOME_PANEL_COLOR, 1);
  }
  game_lcd_draw_text(8, 218, "HOME SWEET HOME", GAME_REF_COLOR_YELLOW, HOME_PANEL_COLOR, 1);
}

static bool is_blocked(int16_t px, int16_t py) {
  if (px < GAME_PLAY_AREA_X + 2) return true;
  if (py < GAME_STATUS_BAR_HEIGHT + 8) return true;
  if (px > GAME_SCREEN_WIDTH - PLAYER_W - 2) return true;
  if (py > 194 - PLAYER_H) return true;

  if (px + PLAYER_W > BED_X && px < BED_X + BED_W &&
      py + PLAYER_H > BED_Y && py < BED_Y + BED_H) return true;

  if (px + PLAYER_W > ARMOR_X && px < ARMOR_X + ARMOR_W &&
      py + PLAYER_H > ARMOR_Y && py < ARMOR_Y + ARMOR_H) return true;

  return false;
}

static void redraw_floor_strip(int16_t x, int16_t y, int16_t w, int16_t h) {
  int16_t py;
  for (py = y; py < y + h; py++) {
    if (py < GAME_STATUS_BAR_HEIGHT || py >= 200) continue;
    int16_t stripe = (py - GAME_STATUS_BAR_HEIGHT) % 12;
    uint16_t color = (stripe < 6) ? HOME_FLOOR_LIGHT : HOME_FLOOR_DARK;
    game_lcd_fill_rect(x, py, w, 1, color);
  }
}

static bool rect_intersects(int16_t x, int16_t y, int16_t w, int16_t h,
                            int16_t rx, int16_t ry, int16_t rw, int16_t rh) {
  return !(x + w <= rx || rx + rw <= x || y + h <= ry || ry + rh <= y);
}

static void redraw_patch(int16_t x, int16_t y, int16_t w, int16_t h) {
  if (x < 0) { w += x; x = 0; }
  if (y < GAME_STATUS_BAR_HEIGHT) { h += (y - GAME_STATUS_BAR_HEIGHT); y = GAME_STATUS_BAR_HEIGHT; }
  if (x + w > GAME_SCREEN_WIDTH) w = GAME_SCREEN_WIDTH - x;
  if (y + h > 200) h = 200 - y;
  if (w <= 0 || h <= 0) return;

  redraw_floor_strip(x, y, w, h);

  // Re-draw intersecting furniture
  if (rect_intersects(x, y, w, h, BED_X, BED_Y, BED_W, BED_H)) {
    draw_bed();
  }
  if (rect_intersects(x, y, w, h, ARMOR_X, ARMOR_Y, ARMOR_W, ARMOR_H)) {
    draw_armor_stand();
  }
  if (rect_intersects(x, y, w, h, WINDOW_X, WINDOW_Y, WINDOW_W, WINDOW_H + 4)) {
    draw_window();
  }
  // Walls: left/right/top/bottom borders
  if (rect_intersects(x, y, w, h, 0, GAME_STATUS_BAR_HEIGHT, GAME_SCREEN_WIDTH, 6)) {
    game_lcd_fill_rect(0, GAME_STATUS_BAR_HEIGHT, GAME_SCREEN_WIDTH, 6, HOME_WALL_COLOR);
  }
  if (rect_intersects(x, y, w, h, 0, 194, GAME_SCREEN_WIDTH, 6)) {
    game_lcd_fill_rect(0, 194, GAME_SCREEN_WIDTH, 6, HOME_WALL_DARK);
  }
  if (rect_intersects(x, y, w, h, 0, GAME_STATUS_BAR_HEIGHT, 6, 200 - GAME_STATUS_BAR_HEIGHT)) {
    game_lcd_fill_rect(0, GAME_STATUS_BAR_HEIGHT, 6, 200 - GAME_STATUS_BAR_HEIGHT, HOME_WALL_COLOR);
  }
  if (rect_intersects(x, y, w, h, GAME_SCREEN_WIDTH - 6, GAME_STATUS_BAR_HEIGHT, 6, 200 - GAME_STATUS_BAR_HEIGHT)) {
    game_lcd_fill_rect(GAME_SCREEN_WIDTH - 6, GAME_STATUS_BAR_HEIGHT, 6, 200 - GAME_STATUS_BAR_HEIGHT, HOME_WALL_COLOR);
  }
}

void game_home_scene_update(void) {
  static bool initialized = false;
  static int16_t player_x = 112;
  static int16_t player_y = 120;
  static int16_t prev_player_x = 112;
  static int16_t prev_player_y = 120;
  static uint32_t last_move_ms = 0;
  static bool prev_near_bed = false;
  static bool in_sleep_anim = false;
  static uint32_t sleep_start_ms = 0;

  bool moved = false;
  bool need_full = false;

  if (s_home_force_full_redraw) {
    initialized = false;
    s_home_force_full_redraw = false;
  }

  if (!initialized) {
    game_character_renderer_init();
    player_x = 112;
    player_y = 120;
    prev_player_x = player_x;
    prev_player_y = player_y;
    initialized = true;
    need_full = true;
  }

  game_input_update();

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
      int16_t candidate_x = next_x;
      int16_t candidate_y = player_y;
      if (!is_blocked(candidate_x, candidate_y)) {
        player_x = candidate_x;
        moved = true;
      }

      candidate_x = player_x;
      candidate_y = next_y;
      if (!is_blocked(candidate_x, candidate_y)) {
        player_y = candidate_y;
        moved = true;
      }
    }
  }

  bool near_bed = is_near_bed(player_x, player_y);

  // Sleep animation takes over the screen
  if (in_sleep_anim) {
    uint32_t elapsed = game_platform_millis() - sleep_start_ms;
    if (elapsed < 500) {
      // Fade to black
      game_lcd_fill_rect(0, GAME_STATUS_BAR_HEIGHT,
                         GAME_SCREEN_WIDTH, GAME_PLAY_AREA_HEIGHT, 0x0000);
    } else if (elapsed < 1500) {
      // Show day counter and restored message
      game_lcd_fill_rect(0, GAME_STATUS_BAR_HEIGHT,
                         GAME_SCREEN_WIDTH, GAME_PLAY_AREA_HEIGHT, 0x0000);
      game_shared_resources_t* shared = game_app_shared_resources();
      uint16_t day = (shared != 0) ? shared->day : 1;
      char day_str[16];
      snprintf(day_str, sizeof(day_str), "DAY %u", (unsigned)day);
      game_lcd_draw_text(85, 90, day_str, GAME_REF_COLOR_WHITE, 0x0000, 2);
      game_lcd_draw_text(45, 125, "HP RESTORED", GAME_REF_COLOR_YELLOW, 0x0000, 2);
      game_lcd_draw_text(50, 155, "ZZZ...", GAME_REF_COLOR_CYAN, 0x0000, 2);
    } else {
      // Wake up
      in_sleep_anim = false;
      need_full = true;
    }
  } else {
    // Normal input handling
    if (near_bed && game_input_button_a_edge()) {
      in_sleep_anim = true;
      sleep_start_ms = game_platform_millis();
      game_shared_resources_t* shared = game_app_shared_resources();
      if (shared != 0) {
        shared->player_hp = PLAYER_MAX_HP;
        shared->day += 1;
      }
    }

    if (game_input_button_b_edge()) {
      game_app_request_scene(GAME_SCENE_WORLD);
      return;
    }

    if (moved) {
      int16_t dx = (int16_t)(player_x - prev_player_x);
      int16_t dy = (int16_t)(player_y - prev_player_y);

      // Precise erase: restore only exposed strips of old position
      if (dx > 0) {
        redraw_patch(prev_player_x - 1, prev_player_y - 1, dx + 2, PLAYER_H + 4);
      } else if (dx < 0) {
        redraw_patch(player_x + PLAYER_W, prev_player_y - 1, -dx + 2, PLAYER_H + 4);
      }

      if (dy > 0) {
        redraw_patch(prev_player_x - 1, prev_player_y - 1, PLAYER_W + 4, dy + 2);
      } else if (dy < 0) {
        redraw_patch(prev_player_x - 1, player_y + PLAYER_H, PLAYER_W + 4, -dy + 2);
      }

      // Draw player at new position
      game_character_renderer_draw(player_x, player_y);

      prev_player_x = player_x;
      prev_player_y = player_y;
    }
  }

  if (!in_sleep_anim && near_bed != prev_near_bed) {
    draw_home_hud(near_bed);
    prev_near_bed = near_bed;
  }

  if (need_full) {
    game_lcd_fill_screen(HOME_FLOOR_DARK);
    draw_floor();
    draw_walls();
    draw_window();
    draw_bed();
    draw_armor_stand();
    game_character_renderer_draw(player_x, player_y);
    draw_home_hud(near_bed);
    prev_player_x = player_x;
    prev_player_y = player_y;
    prev_near_bed = near_bed;
  }
}

void game_home_scene_request_full_redraw(void) {
  s_home_force_full_redraw = true;
}
