#include "game2/scenes/cg_scene.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "game2/app/game_app.h"
#include "game2/config/reference/reference_style.h"
#include "game2/core/scene_id.h"
#include "game2/drivers/lcd_driver.h"
#include "game2/drivers/platform_backend.h"
#include "game2/ui/character_renderer.h"

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------
#define CG_FRAME_MS   120
#define CG_BATTLE_MS  4500
#define CG_SLEEPW_MS  1500
#define CG_SLEEPI_MS  2500
#define CG_FARM_MS    4000
#define CG_FISH_MS    4000
#define CG_TEXT_MS    6500

// ---------------------------------------------------------------------------
// Wasteland palette
// ---------------------------------------------------------------------------
#define WL_DARK       0x18C6
#define WL_DUST       0x2104
#define WL_SKY        0x6A6B
#define WL_SKY_TOP    0x4A49
#define WL_GROUND     0x4208
#define WL_GROUND2    0x528A
#define WL_GROUND3    0x632C
#define WL_WATER      0x033D
#define WL_WATER_MID  0x04BF
#define WL_RIPPLE_C   0x14DF
#define WL_SHORE      0xA367
#define WL_RUST       0xA2A0
#define WL_METAL      0x8410
#define WL_METAL2     0x94B2
#define WL_FIRE       0xFD20
#define WL_GREEN      0x3BA0
#define WL_GREEN_L    0x7BE0
#define WL_WHEAT      0xFE00
#define WL_BONE       0xD6DA
#define WL_LAMP       0xFEA0
#define WL_LASER      0xF800
#define WL_EYE        0xF800
#define WL_ALIEN      0x7BE0
#define WL_WHITE      0xFFFF
#define WL_BLACK      0x0000
#define WL_WIRE       0x7BEF
#define WL_SMOKE      0x8410
#define WL_CLOUD      0x4A29
#define WL_WINDOW     0xFFE0
#define WL_WALL       0x2104
#define WL_CEIL       0x18C3
#define WL_FLOOR      0x4208
#define WL_BED        0x632C

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
typedef enum {
  CG_PHASE_BATTLE = 0,
  CG_PHASE_SLEEP_WALK,
  CG_PHASE_SLEEP_INSIDE,
  CG_PHASE_FARM,
  CG_PHASE_FISH,
  CG_PHASE_TEXT,
  CG_PHASE_DONE
} cg_phase_t;

static bool s_cg_force_redraw = false;

void game_cg_scene_request_full_redraw(void) { s_cg_force_redraw = true; }

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void draw_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
  int16_t x = 0, y = r, d = 3 - 2 * r;
  while (x <= y) {
    game_lcd_draw_pixel((int16_t)(cx + x), (int16_t)(cy + y), color);
    game_lcd_draw_pixel((int16_t)(cx - x), (int16_t)(cy + y), color);
    game_lcd_draw_pixel((int16_t)(cx + x), (int16_t)(cy - y), color);
    game_lcd_draw_pixel((int16_t)(cx - x), (int16_t)(cy - y), color);
    game_lcd_draw_pixel((int16_t)(cx + y), (int16_t)(cy + x), color);
    game_lcd_draw_pixel((int16_t)(cx - y), (int16_t)(cy + x), color);
    game_lcd_draw_pixel((int16_t)(cx + y), (int16_t)(cy - x), color);
    game_lcd_draw_pixel((int16_t)(cx - y), (int16_t)(cy - x), color);
    if (d < 0) { d += 4 * x + 6; } else { d += 4 * (x - y) + 10; y--; }
    x++;
  }
}

static void draw_filled_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
  int16_t x = 0, y = r, d = 3 - 2 * r;
  while (x <= y) {
    game_lcd_fill_rect((int16_t)(cx - x), (int16_t)(cy - y), (int16_t)(2 * x + 1), 1, color);
    game_lcd_fill_rect((int16_t)(cx - y), (int16_t)(cy - x), (int16_t)(2 * y + 1), 1, color);
    game_lcd_fill_rect((int16_t)(cx - x), (int16_t)(cy + y), (int16_t)(2 * x + 1), 1, color);
    game_lcd_fill_rect((int16_t)(cx - y), (int16_t)(cy + x), (int16_t)(2 * y + 1), 1, color);
    if (d < 0) { d += 4 * x + 6; } else { d += 4 * (x - y) + 10; y--; }
    x++;
  }
}

static void draw_alien_2x(int16_t px, int16_t py, bool eye_open) {
  static const uint8_t body[8] = {0x00,0x3C,0x7E,0xFF,0xBD,0xFF,0x66,0x42};
  static const uint8_t eyes[8] = {0x00,0x00,0x24,0x00,0x00,0x00,0x00,0x00};
  static const uint8_t claws[8]= {0x00,0x00,0x00,0x81,0x42,0x81,0x00,0x00};
  int row, col;
  for (row = 0; row < 8; row++) {
    uint8_t b = body[row];
    for (col = 0; col < 8; col++) if ((b >> (7 - col)) & 1)
      game_lcd_fill_rect((int16_t)(px + col * 2), (int16_t)(py + row * 2), 2, 2, WL_ALIEN);
  }
  if (eye_open) {
    for (row = 0; row < 8; row++) {
      uint8_t e = eyes[row];
      for (col = 0; col < 8; col++) if ((e >> (7 - col)) & 1)
        game_lcd_fill_rect((int16_t)(px + col * 2), (int16_t)(py + row * 2), 2, 2, WL_EYE);
    }
  }
  for (row = 0; row < 8; row++) {
    uint8_t c = claws[row];
    for (col = 0; col < 8; col++) if ((c >> (7 - col)) & 1)
      game_lcd_fill_rect((int16_t)(px + col * 2), (int16_t)(py + row * 2), 2, 2, WL_WHITE);
  }
}

static void draw_player_shadow(int16_t px, int16_t py) {
  game_lcd_fill_rect((int16_t)(px + 4), (int16_t)(py + 14), 8, 3, WL_DUST);
}

static void draw_farm_tile_rich(int16_t x, int16_t y, bool has_sprout, bool has_wheat) {
  game_lcd_fill_rect(x, y, 46, 46, WL_GROUND);
  game_lcd_draw_rect(x, y, 46, 46, WL_GROUND2);
  game_lcd_fill_rect((int16_t)(x + 8), (int16_t)(y + 24), 30, 1, WL_GROUND2);
  game_lcd_fill_rect((int16_t)(x + 22), (int16_t)(y + 10), 1, 26, WL_GROUND2);
  game_lcd_fill_rect((int16_t)(x + 5), (int16_t)(y + 35), 3, 2, WL_GROUND2);
  game_lcd_fill_rect((int16_t)(x + 38), (int16_t)(y + 38), 4, 2, WL_GROUND2);
  if (has_sprout) {
    game_lcd_fill_rect((int16_t)(x + 20), (int16_t)(y + 18), 2, 12, WL_GREEN);
    game_lcd_draw_pixel((int16_t)(x + 18), (int16_t)(y + 20), WL_GREEN);
    game_lcd_draw_pixel((int16_t)(x + 24), (int16_t)(y + 22), WL_GREEN);
  }
  if (has_wheat) {
    game_lcd_fill_rect((int16_t)(x + 20), (int16_t)(y + 8), 2, 22, WL_GREEN);
    game_lcd_fill_rect((int16_t)(x + 18), (int16_t)(y + 6), 6, 4, WL_WHEAT);
    game_lcd_fill_rect((int16_t)(x + 19), (int16_t)(y + 4), 4, 2, WL_WHEAT);
  }
}

static void draw_kettle(int16_t x, int16_t y) {
  game_lcd_fill_rect(x, y, 12, 10, WL_METAL2);
  game_lcd_fill_rect((int16_t)(x + 10), (int16_t)(y + 2), 4, 2, WL_METAL2);
}

static void draw_dock_cg(int16_t x, int16_t y) {
  game_lcd_fill_rect(x, y, 52, 32, 0x8A62);
  game_lcd_fill_rect((int16_t)(x + 4), (int16_t)(y + 8), 44, 2, 0x6241);
  game_lcd_fill_rect((int16_t)(x + 4), (int16_t)(y + 18), 44, 2, 0x6241);
  game_lcd_fill_rect((int16_t)(x + 8), (int16_t)(y + 34), 6, 12, 0x6241);
  game_lcd_fill_rect((int16_t)(x + 38), (int16_t)(y + 34), 6, 12, 0x6241);
}

static void draw_lily_pad(int16_t x, int16_t y) {
  draw_filled_circle(x, y, 7, WL_GREEN);
  game_lcd_fill_rect((int16_t)(x + 5), (int16_t)(y - 7), 3, 14, WL_WATER);
}

static void draw_fish(int16_t x, int16_t y) {
  game_lcd_fill_rect(x, y, 16, 10, WL_GREEN_L);
  game_lcd_fill_rect((int16_t)(x - 5), (int16_t)(y + 2), 5, 6, WL_GREEN_L);
  game_lcd_fill_rect((int16_t)(x + 2), (int16_t)(y - 2), 10, 2, WL_GREEN_L);
  game_lcd_draw_pixel((int16_t)(x + 12), (int16_t)(y + 3), WL_BLACK);
  game_lcd_draw_pixel((int16_t)(x + 13), (int16_t)(y + 3), WL_BLACK);
  game_lcd_draw_pixel((int16_t)(x + 8), (int16_t)(y + 5), WL_FIRE);
  game_lcd_draw_pixel((int16_t)(x + 10), (int16_t)(y + 5), WL_FIRE);
}


// ---------------------------------------------------------------------------
// Phase 1: Battle
// ---------------------------------------------------------------------------
#define BAT_STAR_COUNT 28
static void render_battle(uint32_t elapsed, bool first_frame) {
  static int16_t prev_px = 30;
  static int16_t prev_old_px = 30;
  static bool prev_eye = true;
  static int16_t prev_laser_y = -1;
  static int16_t prev_laser_px = -1;
  static int16_t prev_shock_r = -1;
  static int16_t prev_trail_x = -1;
  static uint8_t star_phase[BAT_STAR_COUNT] = {0};

  int16_t pbase = 30;
  int16_t ax = 178;
  int16_t gy = 148;
  int16_t py = (int16_t)(gy - 16);
  int16_t ay = (int16_t)(gy - 16);

  if (first_frame) {
    game_lcd_fill_screen(WL_DARK);
    int dl;
    for (dl = 0; dl < 6; dl++)
      game_lcd_fill_rect(0, (int16_t)(20 + dl * 18), 240, 1, 0x2104);
    // left ruin
    game_lcd_fill_rect(4, 70, 28, 55, WL_METAL);
    game_lcd_fill_rect(8, 55, 8, 18, WL_METAL2);
    game_lcd_fill_rect(18, 62, 10, 12, WL_METAL);
    game_lcd_fill_rect(6, 90, 6, 6, WL_RUST);
    game_lcd_fill_rect(20, 100, 8, 4, WL_RUST);
    // right ruin
    game_lcd_fill_rect(204, 60, 32, 65, WL_METAL);
    game_lcd_fill_rect(212, 45, 10, 20, WL_METAL2);
    game_lcd_fill_rect(226, 52, 8, 15, WL_METAL);
    game_lcd_fill_rect(210, 80, 6, 6, WL_RUST);
    game_lcd_fill_rect(224, 95, 8, 4, WL_RUST);
    // broken bridge
    game_lcd_fill_rect(90, 85, 60, 6, WL_METAL);
    game_lcd_fill_rect(100, 75, 8, 12, WL_METAL2);
    game_lcd_fill_rect(130, 78, 6, 10, WL_METAL2);
    // ground
    game_lcd_fill_rect(0, gy, 240, 92, WL_GROUND);
    game_lcd_fill_rect(0, gy, 240, 2, WL_GROUND3);
    game_lcd_fill_rect(25, (int16_t)(gy + 15), 50, 2, WL_GROUND2);
    game_lcd_fill_rect(110, (int16_t)(gy + 35), 70, 2, WL_GROUND2);
    game_lcd_fill_rect(180, (int16_t)(gy + 55), 50, 2, WL_GROUND2);
    game_lcd_fill_rect(60, (int16_t)(gy + 70), 40, 2, WL_GROUND2);
    // crater
    draw_filled_circle(120, (int16_t)(gy + 22), 9, 0x3186);
    game_lcd_fill_rect(110, (int16_t)(gy + 16), 20, 2, WL_GROUND3);
    // debris
    game_lcd_fill_rect(42, (int16_t)(gy + 8), 14, 6, WL_RUST);
    game_lcd_fill_rect(168, (int16_t)(gy + 12), 10, 5, WL_METAL2);
    game_lcd_fill_rect(190, (int16_t)(gy + 6), 16, 4, WL_RUST);
    // barbed wire
    game_lcd_fill_rect(80, (int16_t)(gy + 45), 40, 1, WL_WIRE);
    game_lcd_fill_rect(140, (int16_t)(gy + 60), 30, 1, WL_WIRE);
    int w; for (w = 0; w < 240; w += 8) {
      game_lcd_draw_pixel(w, (int16_t)(gy + 45), WL_WIRE);
      game_lcd_draw_pixel((int16_t)(w + 4), (int16_t)(gy + 60), WL_WIRE);
    }
    // stars
    const int16_t sx[BAT_STAR_COUNT] = {
      10,30,50,70,90,110,130,150,170,190,210,225,20,40,60,80,100,120,140,160,180,200,220,15,55,85,115,185};
    const int16_t sy[BAT_STAR_COUNT] = {
      15,40,10,55,25,65,18,48,32,58,14,44,35,62,22,52,38,68,28,42,20,50,34,26,46,30,56,24};
    int s; for (s = 0; s < BAT_STAR_COUNT; s++) {
      star_phase[s] = (uint8_t)(s % 5);
      game_lcd_draw_pixel(sx[s], sy[s], WL_WHITE);
    }
    prev_px = pbase; prev_old_px = pbase; prev_eye = true;
    prev_laser_y = -1; prev_laser_px = -1; prev_shock_r = -1; prev_trail_x = -1;
  }

  // twinkle stars
  {
    const int16_t sx[BAT_STAR_COUNT] = {
      10,30,50,70,90,110,130,150,170,190,210,225,20,40,60,80,100,120,140,160,180,200,220,15,55,85,115,185};
    const int16_t sy[BAT_STAR_COUNT] = {
      15,40,10,55,25,65,18,48,32,58,14,44,35,62,22,52,38,68,28,42,20,50,34,26,46,30,56,24};
    int idx  = (int)((elapsed / 100) % BAT_STAR_COUNT);
    int idx2 = (int)((elapsed / 100 + 10) % BAT_STAR_COUNT);
    int idx3 = (int)((elapsed / 100 + 20) % BAT_STAR_COUNT);
    star_phase[idx]  = (uint8_t)((star_phase[idx]  + 1) % 5);
    star_phase[idx2] = (uint8_t)((star_phase[idx2] + 1) % 5);
    star_phase[idx3] = (uint8_t)((star_phase[idx3] + 1) % 5);
    game_lcd_draw_pixel(sx[idx],  sy[idx],  (star_phase[idx]  > 2) ? WL_WHITE : WL_DARK);
    game_lcd_draw_pixel(sx[idx2], sy[idx2], (star_phase[idx2] > 2) ? WL_WHITE : WL_DARK);
    game_lcd_draw_pixel(sx[idx3], sy[idx3], (star_phase[idx3] > 2) ? WL_WHITE : WL_DARK);
  }

  // dust particles
  {
    int dp;
    for (dp = 0; dp < 4; dp++) {
      int16_t dpx = (int16_t)(20 + dp * 55 + (elapsed / 60 + dp * 13) % 50);
      int16_t dpy = (int16_t)(35 + dp * 22 + ((elapsed / 80 + dp * 7) % 10));
      game_lcd_draw_pixel(dpx, dpy, 0x2104);
      if (!first_frame && dpx > 20) game_lcd_draw_pixel((int16_t)(dpx - 3), dpy, WL_DARK);
    }
  }

  // player dash
  int16_t offset = 0;
  uint32_t cycle = elapsed % 900;
  if (cycle < 300) offset = (int16_t)(cycle / 18);
  else if (cycle < 600) offset = (int16_t)(16 - (cycle - 300) / 18);
  int16_t px = (int16_t)(pbase + offset);

  // erase old player + trail
  if (!first_frame && px != prev_px) {
    game_lcd_fill_rect(prev_px, (int16_t)(py - 2), 16, 18, WL_DARK);
    game_lcd_fill_rect(prev_px, gy, 16, 4, WL_GROUND);
    game_lcd_fill_rect(prev_px, gy, 16, 2, WL_GROUND3);
    // erase old trail
    if (prev_trail_x >= 0)
      game_lcd_fill_rect(prev_trail_x, (int16_t)(py + 4), 6, 4, WL_DARK);
  }
  // weapon flash when dashing forward
  if (offset > 4 && offset < 12) {
    game_lcd_fill_rect((int16_t)(px + 14), (int16_t)(py + 4), 4, 6, WL_WHITE);
    game_lcd_fill_rect((int16_t)(px + 16), (int16_t)(py + 5), 2, 4, WL_LAMP);
  }
  draw_player_shadow(px, py);
  game_character_renderer_draw(px, py);
  // orange dash trail
  if (offset > 2) {
    int16_t tx = (int16_t)(px - 8);
    if (tx > pbase) {
      game_lcd_fill_rect(tx, (int16_t)(py + 6), 6, 4, WL_FIRE);
      prev_trail_x = tx;
    }
  }
  prev_old_px = prev_px;
  prev_px = px;

  // alien
  draw_alien_2x(ax, ay, true);
  bool eye_open = ((elapsed / 220) % 4) != 0;
  if (first_frame || eye_open != prev_eye) {
    uint16_t ec = eye_open ? WL_EYE : WL_ALIEN;
    game_lcd_fill_rect((int16_t)(ax + 4), (int16_t)(ay + 2), 2, 2, ec);
    game_lcd_fill_rect((int16_t)(ax + 10), (int16_t)(ay + 2), 2, 2, ec);
    prev_eye = eye_open;
  }

  // laser
  int16_t laser_y = -1;
  if (cycle >= 240 && cycle < 360) {
    int16_t lx = (int16_t)(px + 16);
    game_lcd_fill_rect(lx, (int16_t)(py + 4), (int16_t)(ax - lx), 2, WL_LASER);
    laser_y = py + 4;
    if (prev_laser_px < 0) prev_laser_px = px;
  }
  if (cycle >= 600 && cycle < 720) {
    int16_t lx = (int16_t)(px + 16);
    game_lcd_fill_rect(lx, (int16_t)(py + 10), (int16_t)(ax - lx), 2, WL_LASER);
    laser_y = py + 10;
    if (prev_laser_px < 0) prev_laser_px = px;
  }
  // erase laser + restore alien
  if (!first_frame && laser_y < 0 && prev_laser_y >= 0) {
    int16_t lx_e = (int16_t)(prev_laser_px + 16);
    game_lcd_fill_rect(lx_e, prev_laser_y, (int16_t)(ax - lx_e), 2, WL_DARK);
    draw_alien_2x(ax, ay, true);
    uint16_t ec = eye_open ? WL_EYE : WL_ALIEN;
    game_lcd_fill_rect((int16_t)(ax + 4), (int16_t)(ay + 2), 2, 2, ec);
    game_lcd_fill_rect((int16_t)(ax + 10), (int16_t)(ay + 2), 2, 2, ec);
    // erase shockwave
    if (prev_shock_r >= 0) {
      draw_circle(ax, prev_laser_y, prev_shock_r, WL_DARK);
      prev_shock_r = -1;
    }
    prev_laser_px = -1;
  }
  // shockwave when laser hits
  if (laser_y >= 0) {
    int16_t sr = (int16_t)(((cycle % 100) * 8) / 100);
    if (sr > 0 && sr <= 8) {
      if (!first_frame && prev_shock_r >= 0 && prev_shock_r != sr)
        draw_circle(ax, laser_y, prev_shock_r, WL_DARK);
      draw_circle(ax, laser_y, sr, WL_WHITE);
      prev_shock_r = sr;
    }
  }
  prev_laser_y = laser_y;

  // explosion sparks
  if (laser_y >= 0) {
    int seed = (int)(elapsed / 30);
    game_lcd_draw_pixel((int16_t)(ax - 4 + (seed & 7)), (int16_t)(laser_y - 2 + ((seed >> 3) & 5)), WL_FIRE);
    game_lcd_draw_pixel((int16_t)(ax - 8 + ((seed >> 1) & 7)), (int16_t)(laser_y + 1 + ((seed >> 4) & 5)), WL_FIRE);
    game_lcd_draw_pixel((int16_t)(ax - 2 + ((seed >> 2) & 7)), (int16_t)(laser_y - 4 + ((seed >> 5) & 5)), WL_WHITE);
    game_lcd_draw_pixel((int16_t)(ax - 6 + ((seed >> 3) & 7)), (int16_t)(laser_y + 3 + ((seed >> 6) & 5)), WL_LAMP);
  }

  // meteor
  if (elapsed > 600 && elapsed < 1600) {
    uint32_t mt = elapsed - 600;
    int16_t mx = (int16_t)(260 - mt / 4);
    int16_t my = (int16_t)(15 + mt / 10);
    if (mx > 0 && mx < 240 && my > 0 && my < 70) {
      game_lcd_draw_pixel(mx, my, WL_WHITE);
      game_lcd_draw_pixel((int16_t)(mx + 2), (int16_t)(my - 1), WL_WHITE);
      game_lcd_draw_pixel((int16_t)(mx + 4), (int16_t)(my - 2), WL_METAL2);
      game_lcd_draw_pixel((int16_t)(mx + 6), (int16_t)(my - 3), WL_METAL);
      if (!first_frame && mx < 250) game_lcd_draw_pixel((int16_t)(mx + 8), (int16_t)(my - 4), WL_DARK);
    }
  }
}

// ---------------------------------------------------------------------------
// Phase 2: Sleep-Walk (outside bunker)
// ---------------------------------------------------------------------------
#define SLPW_STAR_COUNT 12
static void render_sleep_walk(uint32_t elapsed, bool first_frame) {
  static int16_t prev_px = 45;

  int16_t rpy = 160;

  if (first_frame) {
    game_lcd_fill_screen(WL_DARK);
    // dust
    int dl; for (dl = 0; dl < 4; dl++)
      game_lcd_fill_rect(0, (int16_t)(16 + dl * 14), 240, 1, 0x2104);
    // distant ruins
    game_lcd_fill_rect(0, 62, 240, 18, WL_METAL);
    game_lcd_fill_rect(30, 52, 12, 12, WL_METAL2);
    game_lcd_fill_rect(160, 50, 14, 15, WL_METAL2);
    // dim red moon
    draw_filled_circle(205, 25, 12, 0x7800);
    draw_filled_circle(202, 22, 3, WL_DARK);
    // ground
    game_lcd_fill_rect(0, 178, 240, 62, WL_GROUND);
    game_lcd_fill_rect(0, 178, 240, 2, WL_GROUND3);
    // bunker
    game_lcd_fill_rect(78, 82, 74, 60, 0x4A29);
    game_lcd_fill_rect(72, 70, 86, 16, WL_RUST);
    game_lcd_fill_rect(76, 74, 78, 10, 0x632C);
    game_lcd_fill_rect(88, 98, 22, 20, 0x4208);
    game_lcd_fill_rect(90, 100, 18, 16, WL_LAMP);
    game_lcd_fill_rect(92, 102, 2, 12, 0x4208);
    game_lcd_fill_rect(98, 102, 2, 12, 0x4208);
    game_lcd_fill_rect(104, 102, 2, 12, 0x4208);
    game_lcd_fill_rect(122, 108, 22, 34, WL_METAL);
    game_lcd_fill_rect(124, 110, 18, 30, 0x2104);
    game_lcd_fill_rect(126, 130, 14, 2, WL_RUST);
    // sandbags
    game_lcd_fill_rect(12, 170, 40, 10, 0x6241);
    game_lcd_fill_rect(16, 166, 32, 6, 0x528A);
    game_lcd_fill_rect(20, 162, 24, 6, 0x6241);
    // barrel
    draw_filled_circle(212, 172, 10, WL_RUST);
    game_lcd_fill_rect(202, 162, 20, 4, WL_METAL2);
    // dead tree
    game_lcd_fill_rect(34, 115, 5, 65, 0x4208);
    game_lcd_fill_rect(28, 115, 18, 4, 0x4208);
    // fence
    int fw; for (fw = 0; fw < 240; fw += 10) {
      game_lcd_fill_rect(fw, 176, 2, 14, WL_METAL);
      game_lcd_draw_pixel((int16_t)(fw + 1), 175, WL_WIRE);
    }
    game_lcd_fill_rect(0, 180, 240, 2, WL_WIRE);
    game_lcd_fill_rect(0, 188, 240, 2, WL_WIRE);
    // stars
    const int16_t sx[SLPW_STAR_COUNT] = {20,50,80,110,140,170,200,220,35,65,95,125};
    const int16_t sy[SLPW_STAR_COUNT] = {22,48,12,58,18,42,32,52,65,10,38,62};
    int s; for (s = 0; s < SLPW_STAR_COUNT; s++) game_lcd_draw_pixel(sx[s], sy[s], WL_WHITE);
    prev_px = 45;
  }

  // player walk (pure night bg y=160, safe)
  int16_t px;
  if (elapsed < 1200) {
    int16_t t = (int16_t)(elapsed / 50);
    px = (int16_t)(45 + t);
    if (px > 92) px = 92;
  } else {
    px = 92;
  }

  if (!first_frame && px != prev_px) {
    game_lcd_fill_rect(prev_px, (int16_t)(rpy - 2), 18, 21, WL_DARK);
  }
  if (elapsed < 1200) {
    draw_player_shadow(px, rpy);
    game_character_renderer_draw(px, rpy);
  } else {
    // door lamp on
    game_lcd_fill_rect(128, 106, 4, 4, WL_LAMP);
  }
  prev_px = px;
}

// ---------------------------------------------------------------------------
// Phase 3: Sleep-Inside (bunker interior)
// ---------------------------------------------------------------------------
static void render_sleep_inside(uint32_t elapsed, bool first_frame) {
  static int16_t prev_px = 30;
  static int16_t prev_zy[3] = {-1, -1, -1};
  static uint16_t prev_dim = 0;

  if (first_frame) {
    game_lcd_fill_screen(WL_WALL);
    // slanted ceiling
    game_lcd_fill_rect(0, 0, 240, 50, WL_CEIL);
    game_lcd_fill_rect(0, 50, 240, 15, WL_WALL);
    int16_t cx;
    for (cx = 0; cx < 240; cx += 20) {
      game_lcd_fill_rect(cx, 48, 2, 4, 0x2104);
    }
    // floor boards
    game_lcd_fill_rect(0, 130, 240, 110, WL_FLOOR);
    int16_t fx;
    for (fx = 0; fx < 240; fx += 18) {
      game_lcd_fill_rect(fx, 130, 1, 110, 0x3186);
    }
    // back wall texture
    game_lcd_fill_rect(0, 65, 240, 65, WL_WALL);
    int16_t wx;
    for (wx = 0; wx < 240; wx += 30) {
      game_lcd_fill_rect(wx, 65, 1, 65, 0x18C3);
    }
    // bed
    game_lcd_fill_rect(80, 118, 70, 14, WL_BED);       // frame
    game_lcd_fill_rect(82, 116, 66, 4, WL_METAL);      // headboard
    game_lcd_fill_rect(84, 120, 62, 8, 0x528A);        // mattress
    game_lcd_fill_rect(86, 118, 18, 6, WL_WHITE);      // pillow
    game_lcd_fill_rect(110, 120, 34, 8, 0x528A);       // blanket
    // oil lamp on wall
    game_lcd_fill_rect(32, 55, 4, 12, WL_METAL);
    draw_filled_circle(34, 52, 5, WL_LAMP);
    draw_filled_circle(34, 52, 3, WL_FIRE);
    // small window (outside dark)
    game_lcd_fill_rect(195, 60, 22, 22, WL_DARK);
    game_lcd_fill_rect(197, 62, 18, 18, 0x7800);
    game_lcd_draw_rect(195, 60, 22, 22, WL_METAL);
    game_lcd_fill_rect(205, 60, 2, 22, WL_METAL);
    game_lcd_fill_rect(195, 70, 22, 2, WL_METAL);
    // table
    game_lcd_fill_rect(20, 110, 30, 4, WL_BED);
    game_lcd_fill_rect(24, 114, 4, 16, WL_BED);
    game_lcd_fill_rect(42, 114, 4, 16, WL_BED);
    // bottle on table
    game_lcd_fill_rect(30, 104, 6, 8, WL_WATER_MID);
    game_lcd_fill_rect(32, 102, 2, 2, WL_WHITE);
    prev_px = 30;
    prev_zy[0] = prev_zy[1] = prev_zy[2] = -1;
    prev_dim = 0;
  }

  // player walk to bed then lie down
  int16_t px;
  bool lying = false;
  if (elapsed < 800) {
    int16_t t = (int16_t)(elapsed / 55);
    px = (int16_t)(30 + t);
    if (px > 88) px = 88;
  } else {
    px = 88;
    lying = true;
  }

  if (!first_frame) {
    if (prev_px != px || lying) {
      // erase old player on floor (floor color)
      game_lcd_fill_rect(prev_px, (int16_t)(114 - 2), 18, 21, WL_FLOOR);
      // restore floor board lines if any
      int16_t fx;
      for (fx = 0; fx < 240; fx += 18) {
        if (fx >= prev_px && fx < prev_px + 18)
          game_lcd_fill_rect(fx, 114, 1, 16, 0x3186);
      }
    }
  }

  if (!lying) {
    draw_player_shadow(px, 114);
    game_character_renderer_draw(px, 114);
  } else {
    // lying on bed (draw over mattress area)
    game_lcd_fill_rect(90, 122, 20, 6, GAME_REF_CHAR_SUIT_COLOR);
    game_lcd_fill_rect(92, 121, 16, 2, GAME_REF_CHAR_ARMOR_COLOR);
  }
  prev_px = px;

  // Zzz floating up (pure ceiling area y=20..100, safe)
  int z;
  for (z = 0; z < 3; z++) {
    uint32_t zs = 1200 + z * 400;
    if (elapsed > zs) {
      int16_t zy = (int16_t)(100 - ((elapsed - zs) / 60));
      if (zy > 20) {
        int16_t zx = (int16_t)(100 + z * 14);
        if (!first_frame && prev_zy[z] >= 0 && prev_zy[z] != zy) {
          game_lcd_fill_rect(zx, prev_zy[z], 10, 12, WL_CEIL);
        }
        game_lcd_draw_text(zx, zy, "Z", WL_WHITE, WL_CEIL, 1);
        prev_zy[z] = zy;
      }
    }
  }

  // vignette
  if (elapsed > 1800) {
    uint16_t dim = (uint16_t)((elapsed - 1800) / 40);
    if (dim > 8) dim = 8;
    if (dim > prev_dim) {
      uint16_t i;
      for (i = prev_dim; i < dim; i++) {
        game_lcd_draw_rect((int16_t)i, (int16_t)i, (int16_t)(240 - i * 2), (int16_t)(240 - i * 2), WL_BLACK);
      }
      prev_dim = dim;
    }
  }
}


// ---------------------------------------------------------------------------
// Phase 4: Farm
// ---------------------------------------------------------------------------
static void render_farm(uint32_t elapsed, bool first_frame) {
  static int16_t prev_drop_y = -1;
  static int16_t prev_grow = -1;
  static int16_t prev_cloud_x[3] = {20, 90, 170};
  static int16_t prev_bx = -1;
  static int16_t prev_by = -1;

  if (first_frame) {
    game_lcd_fill_screen(WL_SKY);
    draw_filled_circle(28, 26, 10, 0xFE00);
    game_lcd_fill_rect(0, 66, 240, 19, 0x528A);
    draw_filled_circle(50, 66, 24, 0x528A);
    draw_filled_circle(170, 66, 30, 0x528A);
    game_lcd_fill_rect(0, 85, 240, 155, WL_GROUND);
    game_lcd_fill_rect(0, 85, 240, 3, WL_GREEN);
    game_lcd_fill_rect(0, 156, 240, 6, WL_WATER);
    game_lcd_fill_rect(0, 154, 240, 2, WL_WATER_MID);
    int fx;
    for (fx = 0; fx < 240; fx += 24) {
      game_lcd_fill_rect(fx, 83, 2, 18, WL_METAL);
    }
    game_lcd_fill_rect(0, 87, 240, 2, WL_WIRE);
    game_lcd_fill_rect(0, 94, 240, 2, WL_WIRE);
    draw_farm_tile_rich(25, 108, false, false);
    draw_farm_tile_rich(92, 108, true,  false);
    draw_farm_tile_rich(159, 108, false, true);
    game_lcd_fill_rect(177, 96, 2, 14, WL_GREEN);
    game_lcd_fill_rect(175, 92, 6, 6, WL_GREEN_L);
    game_lcd_fill_rect(176, 90, 4, 4, WL_FIRE);
    game_lcd_fill_rect(208, 100, 4, 40, WL_METAL);
    draw_filled_circle(210, 96, 5, WL_GREEN_L);
    draw_filled_circle(210, 96, 3, WL_FIRE);
    game_lcd_fill_rect(8, 120, 2, 20, WL_BONE);
    game_lcd_fill_rect(4, 118, 10, 4, WL_BONE);
    game_lcd_fill_rect(2, 116, 4, 6, WL_RUST);
    game_lcd_fill_rect(10, 116, 4, 6, WL_RUST);
    draw_filled_circle(9, 112, 4, WL_BONE);
    game_lcd_fill_rect(220, 138, 3, 24, WL_RUST);
    game_lcd_fill_rect(216, 136, 11, 4, WL_METAL2);
    draw_player_shadow(112, 80);
    game_character_renderer_draw(112, 80);
    int c;
    for (c = 0; c < 3; c++) {
      prev_cloud_x[c] = (int16_t)(20 + c * 75);
      int16_t cx = prev_cloud_x[c];
      game_lcd_fill_rect(cx, 12, 34, 10, WL_CLOUD);
      game_lcd_fill_rect((int16_t)(cx + 5), 8, 22, 8, WL_CLOUD);
    }
    prev_drop_y = -1; prev_grow = -1; prev_bx = -1; prev_by = -1;
  }

  {
    int c;
    for (c = 0; c < 3; c++) {
      int16_t new_cx = (int16_t)(20 + c * 75 + (elapsed / 100) % 90);
      if (!first_frame && new_cx != prev_cloud_x[c]) {
        int16_t old_cx = prev_cloud_x[c];
        game_lcd_fill_rect(old_cx, 8, 40, 14, WL_SKY);
      }
      game_lcd_fill_rect(new_cx, 12, 34, 10, WL_CLOUD);
      game_lcd_fill_rect((int16_t)(new_cx + 5), 8, 22, 8, WL_CLOUD);
      prev_cloud_x[c] = new_cx;
    }
  }

  if (elapsed > 600 && elapsed < 2100) {
    draw_kettle(126, 66);
    uint32_t drop_t = (elapsed - 600) % 350;
    if (drop_t < 260) {
      int16_t dy = (int16_t)(100 + drop_t / 12);
      if (!first_frame && prev_drop_y >= 0 && prev_drop_y != dy) {
        game_lcd_fill_rect(132, prev_drop_y, 2, 2, WL_GROUND);
      }
      game_lcd_fill_rect(132, dy, 2, 2, WL_WATER_MID);
      prev_drop_y = dy;
    } else if (!first_frame && prev_drop_y >= 0) {
      game_lcd_fill_rect(132, prev_drop_y, 2, 2, WL_GROUND);
      prev_drop_y = -1;
    }
  } else if (!first_frame && prev_drop_y >= 0) {
    game_lcd_fill_rect(132, prev_drop_y, 2, 2, WL_GROUND);
    prev_drop_y = -1;
  }

  if (!first_frame && elapsed >= 2100 && elapsed < 2200) {
    game_lcd_fill_rect(126, 66, 16, 10, WL_SKY);
  }

  if (elapsed > 1400 && elapsed < 2100) {
    int16_t grow = (int16_t)((elapsed - 1400) / 35);
    if (first_frame || grow != prev_grow) {
      int16_t stem_h = (int16_t)(grow + 4);
      int16_t stem_y = (int16_t)(128 - grow);
      game_lcd_fill_rect(112, stem_y, 2, stem_h, WL_GREEN);
      if (grow > 3) {
        game_lcd_fill_rect(110, (int16_t)(stem_y + 2), 2, 2, WL_GREEN);
        game_lcd_fill_rect(116, (int16_t)(stem_y + 4), 2, 2, WL_GREEN);
      }
      if (grow > 8) {
        game_lcd_fill_rect(109, (int16_t)(stem_y - 2), 4, 4, WL_FIRE);
      }
      prev_grow = grow;
    }
  }
  if (first_frame && elapsed >= 2100) {
    game_lcd_fill_rect(112, 106, 2, 26, WL_GREEN);
    game_lcd_fill_rect(110, 104, 6, 4, WL_GREEN_L);
    game_lcd_fill_rect(111, 102, 4, 4, WL_FIRE);
  }

  if (elapsed > 2600 && elapsed < 3600) {
    int16_t bx = (int16_t)(250 - (elapsed - 2600) / 3);
    int16_t by = (int16_t)(62 + ((elapsed / 100) % 2) * 5);
    if (bx < 240 && bx > -10) {
      if (!first_frame && prev_bx >= 0) {
        game_lcd_fill_rect((int16_t)(prev_bx - 1), (int16_t)(prev_by - 1), 6, 6, WL_SKY);
      }
      game_lcd_draw_pixel(bx, by, WL_GREEN_L);
      game_lcd_draw_pixel((int16_t)(bx + 2), (int16_t)(by - 1), WL_FIRE);
      game_lcd_draw_pixel((int16_t)(bx + 2), (int16_t)(by + 1), WL_FIRE);
      prev_bx = bx; prev_by = by;
    }
  }
}

// ---------------------------------------------------------------------------
// Phase 5: Fish (blue water)
// ---------------------------------------------------------------------------
static void erase_fish_area(int16_t x, int16_t y, int16_t w, int16_t h) {
  if (y + h <= 102) {
    game_lcd_fill_rect(x, y, w, h, WL_SHORE);
  } else if (y >= 102) {
    game_lcd_fill_rect(x, y, w, h, WL_WATER);
  } else {
    int16_t shore_h = 102 - y;
    game_lcd_fill_rect(x, y, w, shore_h, WL_SHORE);
    game_lcd_fill_rect(x, 102, w, (int16_t)(y + h - 102), WL_WATER);
  }
}

static void render_fish_scene(uint32_t elapsed, bool first_frame) {
  static int16_t prev_by = 110;
  static int16_t prev_fy = -1;
  static int16_t prev_fx = 90;
  static bool prev_splash = false;
  static int16_t prev_cloud_x2[2] = {40, 160};
  static int16_t prev_ripple_y[3] = {112, 128, 144};
  static int16_t prev_dragon_x = -1;
  static int16_t prev_dragon_y = -1;

  if (first_frame) {
    game_lcd_fill_rect(0, 0, 240, 80, WL_SKY);
    draw_filled_circle(198, 22, 9, 0xFE00);
    game_lcd_fill_rect(0, 68, 240, 17, 0x528A);
    draw_filled_circle(40, 68, 22, 0x528A);
    draw_filled_circle(155, 68, 28, 0x528A);
    game_lcd_fill_rect(0, 90, 240, 12, WL_SHORE);
    game_lcd_fill_rect(0, 102, 240, 138, WL_WATER);
    draw_dock_cg(12, 90);
    draw_filled_circle(45, 98, 10, WL_BLACK);
    draw_filled_circle(45, 98, 6, WL_SHORE);
    game_lcd_fill_rect(118, 84, 55, 6, WL_RUST);
    game_lcd_fill_rect(120, 82, 51, 2, WL_METAL2);
    game_lcd_fill_rect(220, 78, 5, 30, 0x4208);
    game_lcd_fill_rect(216, 76, 14, 4, 0x4208);
    game_lcd_fill_rect(212, 72, 8, 8, 0x4208);
    draw_lily_pad(128, 152);
    draw_lily_pad(188, 172);
    draw_lily_pad(78, 182);
    int r;
    for (r = 0; r < 6; r++) {
      game_lcd_fill_rect((int16_t)(206 + r * 5), (int16_t)(80 + r * 2), 3, (int16_t)(30 - r * 3), 0x4208);
    }
    game_lcd_fill_rect(60, 140, 30, 2, WL_WATER_MID);
    game_lcd_fill_rect(160, 160, 25, 2, WL_WATER_MID);
    draw_player_shadow(30, 74);
    game_character_renderer_draw(30, 74);
    game_lcd_fill_rect(44, 76, 18, 2, WL_METAL);
    game_lcd_fill_rect(58, 70, 12, 2, WL_METAL);
    int c;
    for (c = 0; c < 2; c++) {
      prev_cloud_x2[c] = (int16_t)(40 + c * 120);
      int16_t cx = prev_cloud_x2[c];
      game_lcd_fill_rect(cx, 10, 30, 8, WL_CLOUD);
      game_lcd_fill_rect((int16_t)(cx + 4), 6, 20, 6, WL_CLOUD);
    }
    prev_by = 110; prev_fy = -1; prev_fx = 90; prev_splash = false;
    prev_ripple_y[0] = 112; prev_ripple_y[1] = 128; prev_ripple_y[2] = 144;
    prev_dragon_x = -1; prev_dragon_y = -1;
  }

  {
    int c;
    for (c = 0; c < 2; c++) {
      int16_t new_cx = (int16_t)(40 + c * 120 + (elapsed / 110) % 70);
      if (!first_frame && new_cx != prev_cloud_x2[c]) {
        int16_t old_cx = prev_cloud_x2[c];
        game_lcd_fill_rect(old_cx, 6, 34, 12, WL_SKY);
      }
      game_lcd_fill_rect(new_cx, 10, 30, 8, WL_CLOUD);
      game_lcd_fill_rect((int16_t)(new_cx + 4), 6, 20, 6, WL_CLOUD);
      prev_cloud_x2[c] = new_cx;
    }
  }

  {
    int ri;
    for (ri = 0; ri < 3; ri++) {
      int16_t new_y = (int16_t)(112 + ri * 18 + ((elapsed / 80 + ri * 5) % 14));
      int16_t rx = (int16_t)(55 + ri * 48);
      if (!first_frame && new_y != prev_ripple_y[ri]) {
        game_lcd_fill_rect(rx, prev_ripple_y[ri], 42, 1, WL_WATER);
      }
      game_lcd_fill_rect(rx, new_y, 42, 1, WL_WATER_MID);
      prev_ripple_y[ri] = new_y;
    }
  }

  int16_t bx = 66;
  int16_t by = 110;
  if (elapsed > 700 && elapsed < 1600) {
    by = (int16_t)(110 + ((elapsed / 100) % 2) * 4);
  }
  if (!first_frame && by != prev_by) {
    game_lcd_fill_rect(bx, prev_by, 5, 5, WL_WATER);
  }
  game_lcd_fill_rect(bx, by, 5, 5, WL_WHITE);
  game_lcd_draw_pixel((int16_t)(bx + 2), (int16_t)(by + 2), WL_EYE);
  prev_by = by;

  int16_t rtx = 70;
  int16_t rty = 70;
  int i;
  for (i = 0; i < 7; i++) {
    int16_t lx = (int16_t)(rtx + (bx - rtx) * i / 6);
    int16_t ly = (int16_t)(rty + (by - rty) * i / 6 + (i == 4 ? 3 : 0));
    game_lcd_draw_pixel(lx, ly, WL_BONE);
  }

  if (elapsed > 500 && elapsed < 1600) {
    int16_t dx = (int16_t)(180 - (elapsed - 500) / 6);
    int16_t dy = (int16_t)(90 + ((elapsed / 90) % 2) * 2);
    if (dx > 70 && dx < 200) {
      if (!first_frame && prev_dragon_x >= 0) {
        game_lcd_fill_rect((int16_t)(prev_dragon_x - 1), (int16_t)(prev_dragon_y - 1), 6, 4, WL_WATER);
      }
      game_lcd_fill_rect(dx, dy, 4, 1, WL_GREEN_L);
      game_lcd_fill_rect((int16_t)(dx - 2), (int16_t)(dy - 1), 2, 1, WL_GREEN_L);
      game_lcd_fill_rect((int16_t)(dx + 4), (int16_t)(dy - 1), 2, 1, WL_GREEN_L);
      prev_dragon_x = dx;
      prev_dragon_y = dy;
    }
  }

  if (elapsed > 1800 && elapsed < 2800) {
    uint32_t t = elapsed - 1800;
    int16_t fy;
    if (t < 500) {
      fy = (int16_t)(124 - t / 10);
    } else {
      fy = (int16_t)(74 + (t - 500) / 10);
    }
    if (fy < 80) fy = 80;
    if (fy > 124) fy = 124;
    int16_t fx = (int16_t)(88 + t / 6);
    if (fx > 220) fx = 220;

    if (!first_frame && prev_fy >= 0 && (fy != prev_fy || fx != prev_fx)) {
      erase_fish_area((int16_t)(prev_fx - 5), (int16_t)(prev_fy - 2), 22, 14);
    }
    draw_fish(fx, fy);
    prev_fy = fy;
    prev_fx = fx;

    if (t > 700 && fy > 110) {
      if (!prev_splash) {
        game_lcd_fill_rect(86, 122, 4, 3, WL_WHITE);
        game_lcd_fill_rect(102, 120, 3, 3, WL_WHITE);
        game_lcd_fill_rect(94, 124, 2, 2, WL_GREEN_L);
        prev_splash = true;
      }
    } else if (prev_splash) {
      game_lcd_fill_rect(86, 122, 4, 3, WL_WATER);
      game_lcd_fill_rect(102, 120, 3, 3, WL_WATER);
      game_lcd_fill_rect(94, 124, 2, 2, WL_WATER);
      prev_splash = false;
    }
  }
}

// ---------------------------------------------------------------------------
// Phase 6: Text
// ---------------------------------------------------------------------------
static void render_text(uint32_t elapsed, bool first_frame) {
  static int prev_c1 = 0;
  static int prev_c2 = 0;
  static bool prev_prompt = false;

  const char* line1 = "In this harsh world,";
  const char* line2 = "supplies are vital.";

  if (first_frame) {
    game_lcd_fill_screen(WL_DUST);
    game_lcd_draw_rect(8, 8, 224, 224, WL_RUST);
    game_lcd_draw_rect(10, 10, 220, 220, WL_METAL);
    game_lcd_draw_rect(14, 14, 212, 212, WL_RUST);
    game_lcd_fill_rect(12, 12, 4, 4, WL_METAL2);
    game_lcd_fill_rect(224, 12, 4, 4, WL_METAL2);
    game_lcd_fill_rect(12, 224, 4, 4, WL_METAL2);
    game_lcd_fill_rect(224, 224, 4, 4, WL_METAL2);
    int d;
    for (d = 0; d < 6; d++) {
      game_lcd_fill_rect(18, (int16_t)(28 + d * 36), 4, 14, WL_RUST);
      game_lcd_fill_rect(218, (int16_t)(28 + d * 36), 4, 14, WL_RUST);
    }
    prev_c1 = 0; prev_c2 = 0; prev_prompt = false;
  }

  int chars1 = (int)(elapsed / 160);
  int chars2 = (int)((elapsed > 1600 ? elapsed - 1600 : 0) / 160);
  int len1 = 0;
  int len2 = 0;
  while (line1[len1]) len1++;
  while (line2[len2]) len2++;
  if (chars1 > len1) chars1 = len1;
  if (chars2 > len2) chars2 = len2;

  if (first_frame || chars1 != prev_c1) {
    char buf[32];
    int i;
    for (i = 0; i < chars1; i++) buf[i] = line1[i];
    buf[chars1] = '\0';
    game_lcd_draw_text(36, 88, buf, WL_LAMP, WL_DUST, 1);
    prev_c1 = chars1;
  }

  if (first_frame || chars2 != prev_c2) {
    char buf[32];
    int i;
    for (i = 0; i < chars2; i++) buf[i] = line2[i];
    buf[chars2] = '\0';
    game_lcd_draw_text(40, 112, buf, WL_LAMP, WL_DUST, 1);
    prev_c2 = chars2;
  }

  bool prompt = ((elapsed / 500) % 2 == 0);
  if (first_frame || prompt != prev_prompt) {
    if (prompt) {
      game_lcd_draw_text(80, 185, "Press A", WL_METAL2, WL_DUST, 1);
    } else {
      game_lcd_fill_rect(80, 185, 50, 10, WL_DUST);
    }
    prev_prompt = prompt;
  }
}

// ---------------------------------------------------------------------------
// Main update
// ---------------------------------------------------------------------------
void game_cg_scene_update(void) {
  static bool initialized = false;
  static uint32_t phase_start_ms = 0;
  static cg_phase_t phase = CG_PHASE_BATTLE;
  static uint32_t last_frame_ms = 0;
  static cg_phase_t prev_phase = CG_PHASE_DONE;

  if (s_cg_force_redraw) {
    initialized = false;
    s_cg_force_redraw = false;
  }

  if (!initialized) {
    game_lcd_init();
    game_character_renderer_init();
    phase = CG_PHASE_BATTLE;
    phase_start_ms = game_platform_millis();
    last_frame_ms = 0;
    prev_phase = CG_PHASE_DONE;
    initialized = true;
  }

  game_input_update();

  uint32_t now = game_platform_millis();
  uint32_t phase_elapsed = now - phase_start_ms;

  if (game_input_button_a_edge() || game_input_button_b_edge()) {
    initialized = false;
    game_app_request_scene(GAME_SCENE_WORLD);
    return;
  }

  if (now - last_frame_ms < (uint32_t)CG_FRAME_MS) {
    return;
  }
  last_frame_ms = now;

  bool first_frame = (phase != prev_phase);

  switch (phase) {
    case CG_PHASE_BATTLE:
      render_battle(phase_elapsed, first_frame);
      if (phase_elapsed >= (uint32_t)CG_BATTLE_MS) {
        prev_phase = phase;
        phase = CG_PHASE_SLEEP_WALK;
        phase_start_ms = now;
      }
      break;
    case CG_PHASE_SLEEP_WALK:
      render_sleep_walk(phase_elapsed, first_frame);
      if (phase_elapsed >= (uint32_t)CG_SLEEPW_MS) {
        prev_phase = phase;
        phase = CG_PHASE_SLEEP_INSIDE;
        phase_start_ms = now;
      }
      break;
    case CG_PHASE_SLEEP_INSIDE:
      render_sleep_inside(phase_elapsed, first_frame);
      if (phase_elapsed >= (uint32_t)CG_SLEEPI_MS) {
        prev_phase = phase;
        phase = CG_PHASE_FARM;
        phase_start_ms = now;
      }
      break;
    case CG_PHASE_FARM:
      render_farm(phase_elapsed, first_frame);
      if (phase_elapsed >= (uint32_t)CG_FARM_MS) {
        prev_phase = phase;
        phase = CG_PHASE_FISH;
        phase_start_ms = now;
      }
      break;
    case CG_PHASE_FISH:
      render_fish_scene(phase_elapsed, first_frame);
      if (phase_elapsed >= (uint32_t)CG_FISH_MS) {
        prev_phase = phase;
        phase = CG_PHASE_TEXT;
        phase_start_ms = now;
      }
      break;
    case CG_PHASE_TEXT:
      render_text(phase_elapsed, first_frame);
      if (phase_elapsed >= (uint32_t)CG_TEXT_MS) {
        prev_phase = phase;
        phase = CG_PHASE_DONE;
        phase_start_ms = now;
      }
      break;
    case CG_PHASE_DONE:
      initialized = false;
      game_app_request_scene(GAME_SCENE_WORLD);
      return;
  }

  if (first_frame) {
    prev_phase = phase;
  }
}
