#include "game2/scenes/fishing_scene.h"

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
// Constants
// ---------------------------------------------------------------------------
#define FISHING_PANEL_COLOR 0x2104

// Dock where the player stands
#define DOCK_X 8
#define DOCK_Y 100
#define DOCK_W 52
#define DOCK_H 36

// Player position on dock
#define PLAYER_X 22
#define PLAYER_Y 108

// Bobber position in pond
#define BOBBER_CX 150
#define BOBBER_CY 112

// Reeling minigame
#define REEL_BOX_X 20
#define REEL_BOX_Y 130
#define REEL_BOX_W 200
#define REEL_BOX_H 44
#define REEL_BAR_X (REEL_BOX_X + 8)
#define REEL_BAR_Y (REEL_BOX_Y + 20)
#define REEL_BAR_W (REEL_BOX_W - 16)
#define REEL_BAR_H 10
#define REEL_ZONE_X (REEL_BAR_X + (REEL_BAR_W * 2) / 3 - REEL_ZONE_W)
#define REEL_ZONE_W 44
#define REEL_LINE_W 3
#define REEL_LINE_Y (REEL_BAR_Y + 1)
#define REEL_LINE_H (REEL_BAR_H - 2)
#define REEL_PUSH_PX 1
#define REEL_DRIFT_PX 1
#define REEL_SUCCESS_MS 2000u

// Timing
#define CAST_DURATION_MS 500u
#define WAIT_MIN_MS 5000u
#define WAIT_MAX_MS 10000u
#define BITE_TIMEOUT_MS 3000u
#define RESULT_DISPLAY_MS 2500u

// Colors
#define COLOR_WATER_DEEP 0x033D
#define COLOR_WATER_LIGHT 0x04BF
#define COLOR_WATER_SPARKLE 0x03F8
#define COLOR_RIPPLE 0x14DF
#define COLOR_SHORE 0xA367
#define COLOR_REED 0x57E0
#define COLOR_REED_DARK 0x3AC0
#define COLOR_DOCK 0x8A62
#define COLOR_DOCK_DARK 0x6241
#define COLOR_DOCK_OUTLINE 0xB596
#define COLOR_LINE 0xCE79
#define COLOR_EXCLAIM 0xFFE0
#define COLOR_REEL_BG 0x18C3
#define COLOR_REEL_BAR_BG 0x4208
#define COLOR_REEL_ZONE 0x07E0
#define COLOR_REEL_LINE 0xF800

// Enhanced scenery colors
#define COLOR_SKY 0x5BDF
#define COLOR_CLOUD 0xCE79
#define COLOR_MOUNTAIN 0x2D41
#define COLOR_MOUNTAIN_LIGHT 0x34A5
#define COLOR_LILYPAD 0x3AA5
#define COLOR_STONE 0x8C71
#define COLOR_BARREL 0x9B83
#define COLOR_ROPE 0xF800

// ---------------------------------------------------------------------------
// State machine
// ---------------------------------------------------------------------------
typedef enum {
  FISHING_PHASE_IDLE = 0,
  FISHING_PHASE_CASTING,
  FISHING_PHASE_WAITING,
  FISHING_PHASE_BITE,
  FISHING_PHASE_REELING,
  FISHING_PHASE_SUCCESS,
  FISHING_PHASE_FAIL
} fishing_phase_t;

static bool s_fishing_force_full_redraw = false;
static uint32_t s_rng_seed = 1;

void game_fishing_scene_request_full_redraw(void) {
  s_fishing_force_full_redraw = true;
}

static uint32_t rng_next(uint32_t* seed) {
  *seed = (*seed * 1103515245u + 12345u);
  return *seed;
}

// ---------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------
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

static void draw_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
  int16_t x = r;
  int16_t y = 0;
  int16_t err = 0;
  while (x >= y) {
    game_lcd_draw_pixel((int16_t)(cx + x), (int16_t)(cy + y), color);
    game_lcd_draw_pixel((int16_t)(cx + y), (int16_t)(cy + x), color);
    game_lcd_draw_pixel((int16_t)(cx - y), (int16_t)(cy + x), color);
    game_lcd_draw_pixel((int16_t)(cx - x), (int16_t)(cy + y), color);
    game_lcd_draw_pixel((int16_t)(cx - x), (int16_t)(cy - y), color);
    game_lcd_draw_pixel((int16_t)(cx - y), (int16_t)(cy - x), color);
    game_lcd_draw_pixel((int16_t)(cx + y), (int16_t)(cy - x), color);
    game_lcd_draw_pixel((int16_t)(cx + x), (int16_t)(cy - y), color);
    y++;
    if (err <= 0) {
      err += 2 * y + 1;
    } else {
      x--;
      err += 2 * (y - x) + 1;
    }
  }
}

static void draw_pond_background(void) {
  int16_t i;

  // -------------------------------------------------------------------------
  // Sky (above the water horizon)
  // -------------------------------------------------------------------------
  game_lcd_fill_rect(0, GAME_STATUS_BAR_HEIGHT, GAME_SCREEN_WIDTH, 28, COLOR_SKY);

  // Clouds (simple overlapping circles/rects)
  // Cloud 1
  game_lcd_fill_rect(30, 30, 24, 8, COLOR_CLOUD);
  game_lcd_fill_rect(24, 33, 8, 5, COLOR_CLOUD);
  game_lcd_fill_rect(50, 33, 8, 5, COLOR_CLOUD);
  // Cloud 2
  game_lcd_fill_rect(140, 28, 30, 8, COLOR_CLOUD);
  game_lcd_fill_rect(134, 31, 8, 5, COLOR_CLOUD);
  game_lcd_fill_rect(164, 31, 8, 5, COLOR_CLOUD);
  // Cloud 3 (small)
  game_lcd_fill_rect(200, 34, 16, 6, COLOR_CLOUD);
  game_lcd_fill_rect(196, 36, 6, 4, COLOR_CLOUD);

  // -------------------------------------------------------------------------
  // Distant mountains / hills
  // -------------------------------------------------------------------------
  // Hill 1 (far left, darker)
  for (i = 0; i < 12; i++) {
    int16_t w = 12 + i * 2;
    game_lcd_fill_rect((int16_t)(10 + i), (int16_t)(48 + i), w, 1, COLOR_MOUNTAIN);
  }
  // Hill 2 (center-left, lighter)
  for (i = 0; i < 10; i++) {
    int16_t w = 16 + i * 3;
    game_lcd_fill_rect((int16_t)(60 + i * 2), (int16_t)(46 + i), w, 1, COLOR_MOUNTAIN_LIGHT);
  }
  // Hill 3 (right side)
  for (i = 0; i < 14; i++) {
    int16_t w = 10 + i * 2;
    game_lcd_fill_rect((int16_t)(160 + i), (int16_t)(50 + i), w, 1, COLOR_MOUNTAIN);
  }

  // -------------------------------------------------------------------------
  // Water
  // -------------------------------------------------------------------------
  game_lcd_fill_rect(0, 56, GAME_SCREEN_WIDTH, (int16_t)(GAME_PLAY_AREA_HEIGHT - 32), COLOR_WATER_DEEP);

  // Water light patches
  draw_filled_circle(140, 110, 55, COLOR_WATER_LIGHT);
  draw_filled_circle(100, 70, 25, COLOR_WATER_SPARKLE);
  draw_filled_circle(180, 140, 22, COLOR_WATER_SPARKLE);
  draw_filled_circle(70, 150, 18, COLOR_WATER_SPARKLE);
  draw_filled_circle(190, 80, 15, COLOR_WATER_SPARKLE);

  // Ripples (concentric circles)
  draw_circle(140, 110, 40, COLOR_RIPPLE);
  draw_circle(140, 110, 25, COLOR_RIPPLE);
  draw_circle(100, 70, 14, COLOR_RIPPLE);
  draw_circle(190, 80, 10, COLOR_RIPPLE);

  // Water wave lines (horizontal dashes)
  game_lcd_fill_rect(80, 90, 18, 1, COLOR_RIPPLE);
  game_lcd_fill_rect(120, 130, 22, 1, COLOR_RIPPLE);
  game_lcd_fill_rect(160, 100, 14, 1, COLOR_RIPPLE);
  game_lcd_fill_rect(60, 160, 16, 1, COLOR_RIPPLE);
  game_lcd_fill_rect(200, 150, 12, 1, COLOR_RIPPLE);

  // Lily pads (small green ovals)
  game_lcd_fill_rect(110, 125, 8, 4, COLOR_LILYPAD);
  game_lcd_fill_rect(170, 95, 6, 3, COLOR_LILYPAD);
  game_lcd_fill_rect(90, 155, 7, 3, COLOR_LILYPAD);

  // -------------------------------------------------------------------------
  // Shore (top edge)
  // -------------------------------------------------------------------------
  game_lcd_fill_rect(0, GAME_STATUS_BAR_HEIGHT, GAME_SCREEN_WIDTH, 6, COLOR_SHORE);

  // Shore stones (small grey blobs near top shore)
  game_lcd_fill_rect(50, 30, 6, 4, COLOR_STONE);
  game_lcd_fill_rect(120, 28, 5, 3, COLOR_STONE);
  game_lcd_fill_rect(180, 32, 7, 4, COLOR_STONE);

  // -------------------------------------------------------------------------
  // Reeds (tall grass at pond edges)
  // -------------------------------------------------------------------------
  // Right side cluster
  game_lcd_fill_rect(222, 60, 3, 18, COLOR_REED);
  game_lcd_fill_rect(230, 55, 3, 22, COLOR_REED);
  game_lcd_fill_rect(216, 140, 3, 16, COLOR_REED);
  game_lcd_fill_rect(228, 150, 3, 14, COLOR_REED);
  game_lcd_fill_rect(224, 100, 2, 12, COLOR_REED_DARK);
  game_lcd_fill_rect(234, 80, 2, 10, COLOR_REED_DARK);

  // Left side cluster (near dock)
  game_lcd_fill_rect(2, 80, 3, 14, COLOR_REED);
  game_lcd_fill_rect(6, 75, 2, 16, COLOR_REED_DARK);
  game_lcd_fill_rect(4, 120, 3, 12, COLOR_REED);

  // Bottom edge reeds
  game_lcd_fill_rect(80, 188, 2, 8, COLOR_REED);
  game_lcd_fill_rect(140, 190, 3, 6, COLOR_REED);
  game_lcd_fill_rect(190, 186, 2, 10, COLOR_REED_DARK);
}

static void draw_dock(void) {
  // Platform body
  game_lcd_fill_rect(DOCK_X, DOCK_Y, DOCK_W, DOCK_H, COLOR_DOCK);
  game_lcd_draw_rect(DOCK_X, DOCK_Y, DOCK_W, DOCK_H, COLOR_DOCK_OUTLINE);

  // Plank lines
  game_lcd_fill_rect(DOCK_X + 3, DOCK_Y + 6, DOCK_W - 6, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 3, DOCK_Y + 14, DOCK_W - 6, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 3, DOCK_Y + 22, DOCK_W - 6, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 3, DOCK_Y + 30, DOCK_W - 6, 1, COLOR_DOCK_DARK);

  // Nail dots on planks
  game_lcd_fill_rect(DOCK_X + 6, DOCK_Y + 5, 1, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 14, DOCK_Y + 5, 1, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 30, DOCK_Y + 5, 1, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 42, DOCK_Y + 5, 1, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 6, DOCK_Y + 13, 1, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 20, DOCK_Y + 13, 1, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 38, DOCK_Y + 13, 1, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 10, DOCK_Y + 21, 1, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 26, DOCK_Y + 21, 1, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 44, DOCK_Y + 21, 1, 1, COLOR_DOCK_DARK);

  // Support posts (3 posts for stability)
  game_lcd_fill_rect(DOCK_X + 4, DOCK_Y + DOCK_H, 4, 14, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + 24, DOCK_Y + DOCK_H, 4, 12, COLOR_DOCK_DARK);
  game_lcd_fill_rect(DOCK_X + DOCK_W - 8, DOCK_Y + DOCK_H, 4, 14, COLOR_DOCK_DARK);

  // Rope railings on left and right edges
  game_lcd_fill_rect(DOCK_X + 1, DOCK_Y + 2, 2, DOCK_H - 4, COLOR_DOCK_OUTLINE);
  game_lcd_fill_rect(DOCK_X + DOCK_W - 3, DOCK_Y + 2, 2, DOCK_H - 4, COLOR_DOCK_OUTLINE);
  // Rope knots
  game_lcd_fill_rect(DOCK_X + 1, DOCK_Y + 8, 2, 2, COLOR_ROPE);
  game_lcd_fill_rect(DOCK_X + 1, DOCK_Y + 20, 2, 2, COLOR_ROPE);
  game_lcd_fill_rect(DOCK_X + DOCK_W - 3, DOCK_Y + 10, 2, 2, COLOR_ROPE);
  game_lcd_fill_rect(DOCK_X + DOCK_W - 3, DOCK_Y + 24, 2, 2, COLOR_ROPE);

  // Small barrel at dock corner
  game_lcd_fill_rect((int16_t)(DOCK_X + DOCK_W + 2), (int16_t)(DOCK_Y + DOCK_H - 12), 8, 10, COLOR_BARREL);
  game_lcd_draw_rect((int16_t)(DOCK_X + DOCK_W + 2), (int16_t)(DOCK_Y + DOCK_H - 12), 8, 10, COLOR_DOCK_DARK);
  game_lcd_fill_rect((int16_t)(DOCK_X + DOCK_W + 2), (int16_t)(DOCK_Y + DOCK_H - 9), 8, 1, COLOR_DOCK_DARK);
  game_lcd_fill_rect((int16_t)(DOCK_X + DOCK_W + 2), (int16_t)(DOCK_Y + DOCK_H - 5), 8, 1, COLOR_DOCK_DARK);

  // Tackle box near player
  game_lcd_fill_rect((int16_t)(DOCK_X + 36), (int16_t)(DOCK_Y + 24), 10, 6, 0x52AA);
  game_lcd_draw_rect((int16_t)(DOCK_X + 36), (int16_t)(DOCK_Y + 24), 10, 6, COLOR_DOCK_DARK);
  game_lcd_fill_rect((int16_t)(DOCK_X + 40), (int16_t)(DOCK_Y + 26), 2, 2, COLOR_DOCK_DARK);
}

static void draw_fishing_rod_and_line(int16_t px, int16_t py, int16_t bobber_x, int16_t bobber_y, bool show_line) {
  // Rod: angled up-right from player
  int16_t rod_x0 = px + 12;
  int16_t rod_y0 = py + 4;
  int16_t rod_x1 = px + 32;
  int16_t rod_y1 = py - 8;

  // Draw rod as short segments
  game_lcd_fill_rect(rod_x0, rod_y0, 10, 2, COLOR_DOCK);
  game_lcd_fill_rect(rod_x0 + 8, rod_y0 - 3, 10, 2, COLOR_DOCK);
  game_lcd_fill_rect(rod_x0 + 16, rod_y0 - 6, 8, 2, COLOR_DOCK);

  if (show_line) {
    // Fishing line from rod tip to bobber (stepped approximation)
    int16_t tip_x = rod_x1;
    int16_t tip_y = rod_y1;
    int16_t mid_x = (int16_t)((tip_x + bobber_x) / 2);
    int16_t mid_y = (int16_t)(tip_y + 4);

    if (tip_x <= mid_x) {
      game_lcd_fill_rect(tip_x, tip_y, (int16_t)(mid_x - tip_x + 1), 1, COLOR_LINE);
    }
    if (mid_y >= tip_y) {
      game_lcd_fill_rect(mid_x, tip_y, 1, (int16_t)(mid_y - tip_y + 1), COLOR_LINE);
    }
    if (mid_x <= bobber_x) {
      game_lcd_fill_rect(mid_x, mid_y, (int16_t)(bobber_x - mid_x + 1), 1, COLOR_LINE);
    }
    if (bobber_y >= mid_y) {
      game_lcd_fill_rect(bobber_x, mid_y, 1, (int16_t)(bobber_y - mid_y + 1), COLOR_LINE);
    }
  }
}

static void draw_bobber(int16_t x, int16_t y, bool active) {
  uint16_t color = active ? 0xF800 : COLOR_EXCLAIM;
  game_lcd_fill_rect((int16_t)(x - 2), (int16_t)(y - 2), 5, 5, color);
  game_lcd_draw_rect((int16_t)(x - 2), (int16_t)(y - 2), 5, 5, GAME_REF_COLOR_WHITE);
  game_lcd_fill_rect((int16_t)(x - 1), (int16_t)(y - 1), 3, 3, GAME_REF_COLOR_WHITE);
}

static void draw_exclamation(int16_t x, int16_t y) {
  game_lcd_draw_text(x, y, "!", COLOR_EXCLAIM, COLOR_WATER_DEEP, 2);
}

static void draw_player_on_dock(void) {
  // Shadow
  game_lcd_fill_rect(PLAYER_X + 4, PLAYER_Y + 14, 8, 3, 0x0841);
  // Character
  game_character_renderer_draw(PLAYER_X, PLAYER_Y);
}

static void draw_reel_frame(void) {
  // Frame background
  game_lcd_fill_rect(REEL_BOX_X, REEL_BOX_Y, REEL_BOX_W, REEL_BOX_H, COLOR_REEL_BG);
  game_lcd_draw_rect(REEL_BOX_X, REEL_BOX_Y, REEL_BOX_W, REEL_BOX_H, GAME_REF_COLOR_WHITE);

  // Title
  game_lcd_draw_text((int16_t)(REEL_BOX_X + 8), (int16_t)(REEL_BOX_Y + 4), "REEL IN!", GAME_REF_COLOR_WHITE, COLOR_REEL_BG, 1);
  game_lcd_draw_text((int16_t)(REEL_BOX_X + 120), (int16_t)(REEL_BOX_Y + 4), "CW SPIN", GAME_REF_COLOR_YELLOW, COLOR_REEL_BG, 1);

  // Bar background
  game_lcd_fill_rect(REEL_BAR_X, REEL_BAR_Y, REEL_BAR_W, REEL_BAR_H, COLOR_REEL_BAR_BG);
  game_lcd_draw_rect(REEL_BAR_X, REEL_BAR_Y, REEL_BAR_W, REEL_BAR_H, GAME_REF_COLOR_WHITE);

  // Success zone (right side)
  game_lcd_fill_rect(REEL_ZONE_X, (int16_t)(REEL_BAR_Y + 1), REEL_ZONE_W, (int16_t)(REEL_BAR_H - 2), COLOR_REEL_ZONE);
  game_lcd_draw_rect(REEL_ZONE_X, (int16_t)(REEL_BAR_Y + 1), REEL_ZONE_W, (int16_t)(REEL_BAR_H - 2), GAME_REF_COLOR_WHITE);
}

static void draw_reel_line(int16_t line_x) {
  game_lcd_fill_rect(line_x, REEL_LINE_Y, REEL_LINE_W, REEL_LINE_H, COLOR_REEL_LINE);
}

static void erase_reel_line(int16_t line_x) {
  int16_t x;
  for (x = line_x; x < line_x + REEL_LINE_W; x++) {
    uint16_t bar_color;
    if (x >= REEL_ZONE_X && x < REEL_ZONE_X + REEL_ZONE_W) {
      bar_color = COLOR_REEL_ZONE;
    } else {
      bar_color = COLOR_REEL_BAR_BG;
    }
    game_lcd_fill_rect(x, REEL_LINE_Y, 1, REEL_LINE_H, bar_color);
  }
}

static void draw_hud(fishing_phase_t phase, const char* status, uint16_t food, uint16_t relic) {
  char food_line[20];
  char relic_line[20];
  const char* action = "";

  switch (phase) {
    case FISHING_PHASE_IDLE:
      action = "A:CAST";
      break;
    case FISHING_PHASE_CASTING:
      action = "...";
      break;
    case FISHING_PHASE_WAITING:
      action = "WAITING";
      break;
    case FISHING_PHASE_BITE:
      action = "A:HOOK!";
      break;
    case FISHING_PHASE_REELING:
      action = "CW SPIN!";
      break;
    case FISHING_PHASE_SUCCESS:
      action = "NICE!";
      break;
    case FISHING_PHASE_FAIL:
      action = "TOUGH LUCK";
      break;
    default:
      break;
  }

  snprintf(food_line, sizeof(food_line), "FOOD:%u", (unsigned)food);
  snprintf(relic_line, sizeof(relic_line), "RELIC:%u", (unsigned)relic);

  // Top-right HUD
  game_lcd_fill_rect(110, 2, 128, 20, GAME_REF_COLOR_BLACK);
  game_lcd_draw_text(112, 3, food_line, GAME_REF_COLOR_YELLOW, GAME_REF_COLOR_BLACK, 1);
  game_lcd_draw_text(170, 3, relic_line, GAME_REF_COLOR_CYAN, GAME_REF_COLOR_BLACK, 1);

  // Bottom action bar
  game_lcd_fill_rect(0, 200, GAME_SCREEN_WIDTH, 40, FISHING_PANEL_COLOR);
  game_lcd_draw_rect(0, 200, GAME_SCREEN_WIDTH, 40, GAME_REF_COLOR_WHITE);
  game_lcd_draw_text(8, 206, action, GAME_REF_COLOR_YELLOW, FISHING_PANEL_COLOR, 1);
  game_lcd_draw_text(104, 206, "B:RETURN", GAME_REF_COLOR_CYAN, FISHING_PANEL_COLOR, 1);
  game_lcd_draw_text(8, 218, status, GAME_REF_COLOR_WHITE, FISHING_PANEL_COLOR, 1);
}

static void render_full_scene(fishing_phase_t phase, bool show_exclaim, int16_t reel_line_x) {
  int16_t bobber_x = BOBBER_CX;
  int16_t bobber_y = BOBBER_CY;

  // Background
  draw_pond_background();
  draw_dock();

  // Rod and line
  bool show_line = (phase == FISHING_PHASE_WAITING) ||
                   (phase == FISHING_PHASE_BITE) ||
                   (phase == FISHING_PHASE_REELING) ||
                   (phase == FISHING_PHASE_SUCCESS) ||
                   (phase == FISHING_PHASE_FAIL);
  draw_fishing_rod_and_line(PLAYER_X, PLAYER_Y, bobber_x, bobber_y, show_line);

  // Bobber
  if (phase == FISHING_PHASE_WAITING || phase == FISHING_PHASE_BITE || phase == FISHING_PHASE_REELING) {
    draw_bobber(bobber_x, bobber_y, phase == FISHING_PHASE_BITE);
  }

  // Player
  draw_player_on_dock();

  // Exclamation
  if (show_exclaim) {
    draw_exclamation((int16_t)(PLAYER_X + 2), (int16_t)(PLAYER_Y - 14));
  }

  // Reeling minigame
  if (phase == FISHING_PHASE_REELING) {
    draw_reel_frame();
    draw_reel_line(reel_line_x);
  }

  // Title bar
  game_lcd_draw_rect(0, 0, GAME_SCREEN_WIDTH, GAME_STATUS_BAR_HEIGHT, GAME_REF_COLOR_WHITE);
  game_lcd_draw_text(8, 7, "POND", GAME_REF_COLOR_CYAN, GAME_REF_COLOR_BLACK, 1);

  // Play area border
  game_lcd_draw_rect(0,
                     GAME_STATUS_BAR_HEIGHT,
                     GAME_SCREEN_WIDTH,
                     GAME_PLAY_AREA_HEIGHT,
                     GAME_REF_COLOR_WHITE);
}

// ---------------------------------------------------------------------------
// Main update
// ---------------------------------------------------------------------------
void game_fishing_scene_update(void) {
  static bool initialized = false;
  static fishing_phase_t phase = FISHING_PHASE_IDLE;
  static uint32_t phase_enter_ms = 0;
  static uint32_t wait_target_ms = 0;
  static uint32_t last_reel_update_ms = 0;
  static int16_t reel_line_x = 0;
  static int16_t reel_prev_line_x = 0;
  static uint32_t reel_in_zone_since_ms = 0;
  static bool reel_in_zone = false;
  static bool led_state = false;
  static uint32_t led_last_toggle_ms = 0;
  static char status_line[28] = "READY TO FISH";

  // Buzzer state
  static bool buzzer_on = false;
  static uint32_t buzzer_toggle_ms = 0;
  static uint8_t melody_idx = 0;
  static uint32_t melody_note_ms = 0;

  game_shared_resources_t* shared;
  bool need_full = false;
  bool need_partial = false;
  fishing_phase_t prev_phase;

  if (s_fishing_force_full_redraw) {
    initialized = false;
    s_fishing_force_full_redraw = false;
  }

  if (!initialized) {
    phase = FISHING_PHASE_IDLE;
    phase_enter_ms = game_platform_millis();
    snprintf(status_line, sizeof(status_line), "READY TO FISH");
    game_character_renderer_init();
    game_character_renderer_set_dir(GAME_CHARACTER_DIR_RIGHT);
    reel_line_x = REEL_BAR_X + 2;
    reel_prev_line_x = reel_line_x;
    reel_in_zone_since_ms = 0;
    reel_in_zone = false;
    led_state = false;
    led_last_toggle_ms = 0;
    game_led_set(false);
    buzzer_on = false;
    buzzer_toggle_ms = 0;
    melody_idx = 0;
    melody_note_ms = 0;
    game_buzzer_stop();
    initialized = true;
    need_full = true;
  }

  prev_phase = phase;
  game_input_update();

  // B button handling
  if (game_input_button_b_edge()) {
    if (phase == FISHING_PHASE_REELING) {
      phase = FISHING_PHASE_FAIL;
      phase_enter_ms = game_platform_millis();
      snprintf(status_line, sizeof(status_line), "REEL CANCELED");
    } else if (phase == FISHING_PHASE_SUCCESS || phase == FISHING_PHASE_FAIL) {
      phase = FISHING_PHASE_IDLE;
      phase_enter_ms = game_platform_millis();
      snprintf(status_line, sizeof(status_line), "READY TO FISH");
    } else {
      game_buzzer_stop();
      initialized = false;
      game_app_request_scene(GAME_SCENE_WORLD);
      return;
    }
  }

  // Phase state machine
  switch (phase) {
    case FISHING_PHASE_IDLE:
      if (game_input_button_a_edge()) {
        phase = FISHING_PHASE_CASTING;
        phase_enter_ms = game_platform_millis();
        snprintf(status_line, sizeof(status_line), "CASTING...");
      }
      break;

    case FISHING_PHASE_CASTING:
      if (game_platform_millis() - phase_enter_ms >= CAST_DURATION_MS) {
        phase = FISHING_PHASE_WAITING;
        phase_enter_ms = game_platform_millis();
        uint32_t rand_val = game_platform_millis() ^ s_rng_seed;
        wait_target_ms = WAIT_MIN_MS + (rand_val % (WAIT_MAX_MS - WAIT_MIN_MS + 1));
        s_rng_seed = rand_val * 1103515245u + 12345u;
        snprintf(status_line, sizeof(status_line), "WAITING...");
      }
      break;

    case FISHING_PHASE_WAITING:
      if (game_platform_millis() - phase_enter_ms >= wait_target_ms) {
        phase = FISHING_PHASE_BITE;
        phase_enter_ms = game_platform_millis();
        snprintf(status_line, sizeof(status_line), "FISH ON! HOOK IT!");
      }
      break;

    case FISHING_PHASE_BITE:
      if (game_platform_millis() - phase_enter_ms >= BITE_TIMEOUT_MS) {
        phase = FISHING_PHASE_FAIL;
        phase_enter_ms = game_platform_millis();
        snprintf(status_line, sizeof(status_line), "FISH GOT AWAY");
      } else if (game_input_button_a_edge()) {
        phase = FISHING_PHASE_REELING;
        phase_enter_ms = game_platform_millis();
        reel_line_x = REEL_BAR_X;
        reel_prev_line_x = reel_line_x;
        reel_in_zone_since_ms = 0;
        reel_in_zone = false;
        last_reel_update_ms = game_platform_millis();
        snprintf(status_line, sizeof(status_line), "REEL! CW SPIN!");
      }
      break;

    case FISHING_PHASE_REELING:
      if (game_platform_millis() - last_reel_update_ms >= 16u) {
        last_reel_update_ms = game_platform_millis();
        reel_prev_line_x = reel_line_x;

        uint8_t push_count = 0;
        while (game_input_cw_pulse() && push_count < 3) {
          reel_line_x = (int16_t)(reel_line_x + 1);
          push_count++;
        }
        if (push_count == 0) {
          reel_line_x = (int16_t)(reel_line_x - REEL_DRIFT_PX);
        }

        if (reel_line_x < REEL_BAR_X + 2) reel_line_x = REEL_BAR_X + 2;
        if (reel_line_x > REEL_BAR_X + REEL_BAR_W - REEL_LINE_W - 2) {
          reel_line_x = (int16_t)(REEL_BAR_X + REEL_BAR_W - REEL_LINE_W - 2);
        }

        if (reel_line_x != reel_prev_line_x) {
          need_partial = true;
        }

        bool now_in_zone = (reel_line_x >= REEL_ZONE_X &&
                            reel_line_x + REEL_LINE_W <= REEL_ZONE_X + REEL_ZONE_W);

        if (now_in_zone && !reel_in_zone) {
          reel_in_zone = true;
          reel_in_zone_since_ms = game_platform_millis();
        } else if (!now_in_zone) {
          reel_in_zone = false;
          reel_in_zone_since_ms = 0;
        }

        if (reel_in_zone && (game_platform_millis() - reel_in_zone_since_ms >= REEL_SUCCESS_MS)) {
          phase = FISHING_PHASE_SUCCESS;
          phase_enter_ms = game_platform_millis();
          shared = game_app_shared_resources();
          if (shared != 0) {
            shared->food = (uint16_t)(shared->food + 1);
            bool get_relic = ((rng_next(&s_rng_seed) & 1u) == 1u);
            if (get_relic) {
              shared->relic = (uint16_t)(shared->relic + 1);
              snprintf(status_line, sizeof(status_line), "GOT FISH + RELIC!");
            } else {
              snprintf(status_line, sizeof(status_line), "GOT FISH!");
            }
          } else {
            snprintf(status_line, sizeof(status_line), "GOT FISH!");
          }
        }
      }
      break;

    case FISHING_PHASE_SUCCESS:
      if (game_platform_millis() - phase_enter_ms >= RESULT_DISPLAY_MS) {
        if (game_input_button_a_edge()) {
          phase = FISHING_PHASE_IDLE;
          phase_enter_ms = game_platform_millis();
          snprintf(status_line, sizeof(status_line), "READY TO FISH");
        }
      }
      break;

    case FISHING_PHASE_FAIL:
      if (game_platform_millis() - phase_enter_ms >= RESULT_DISPLAY_MS) {
        if (game_input_button_a_edge()) {
          phase = FISHING_PHASE_IDLE;
          phase_enter_ms = game_platform_millis();
          snprintf(status_line, sizeof(status_line), "READY TO FISH");
        }
      }
      break;

    default:
      break;
  }

  // LED blink + buzzer during BITE and REELING
  {
    uint32_t now_ms = game_platform_millis();
    if (phase == FISHING_PHASE_BITE) {
      // Warning beep: 1200Hz, 150ms on / 150ms off
      if (now_ms - buzzer_toggle_ms >= 150u) {
        buzzer_on = !buzzer_on;
        if (buzzer_on) {
          game_buzzer_tone(1200, 150);
        } else {
          game_buzzer_stop();
        }
        buzzer_toggle_ms = now_ms;
      }
    } else if (phase == FISHING_PHASE_REELING) {
      // Reeling sound: soft intermittent low-frequency chirp
      // 350Hz for 120ms on, 80ms off
      if (now_ms - buzzer_toggle_ms >= 120u) {
        buzzer_on = !buzzer_on;
        if (buzzer_on) {
          game_buzzer_tone(350, 120);
        } else {
          game_buzzer_stop();
        }
        buzzer_toggle_ms = now_ms;
      }
    } else if (phase == FISHING_PHASE_SUCCESS) {
      // Mario course-clear melody: C5-E5-G5-C6
      static const uint16_t s_melody_freqs[] = {523, 659, 784, 1047, 0};
      static const uint16_t s_melody_durs[]  = {150, 150, 150, 500, 0};
      if (melody_idx == 0 && melody_note_ms == 0) {
        melody_note_ms = now_ms;
        game_buzzer_tone(s_melody_freqs[0], s_melody_durs[0]);
      } else if (s_melody_freqs[melody_idx] != 0) {
        if (now_ms - melody_note_ms >= s_melody_durs[melody_idx]) {
          melody_idx++;
          melody_note_ms = now_ms;
          if (s_melody_freqs[melody_idx] != 0) {
            game_buzzer_tone(s_melody_freqs[melody_idx], s_melody_durs[melody_idx]);
          } else {
            game_buzzer_stop();
          }
        }
      }
    } else if (phase == FISHING_PHASE_FAIL) {
      // Low fail tone
      if (buzzer_on) {
        buzzer_on = false;
        game_buzzer_tone(200, 400);
      }
    } else {
      // IDLE / WAITING / CASTING: silence
      if (buzzer_on) {
        buzzer_on = false;
        game_buzzer_stop();
      }
      melody_idx = 0;
      melody_note_ms = 0;
    }

    // LED blink during BITE and REELING
    if (phase == FISHING_PHASE_BITE || phase == FISHING_PHASE_REELING) {
      if (now_ms - led_last_toggle_ms >= 200u) {
        led_state = !led_state;
        game_led_set(led_state);
        led_last_toggle_ms = now_ms;
      }
    } else {
      if (led_state) {
        led_state = false;
        game_led_set(false);
      }
    }
  }

  // Phase change always triggers a full redraw
  if (phase != prev_phase) {
    need_full = true;
    // Reset melody on phase transitions out of SUCCESS
    if (prev_phase == FISHING_PHASE_SUCCESS) {
      melody_idx = 0;
      melody_note_ms = 0;
      game_buzzer_stop();
    }
  }

  // Rendering
  if (need_full) {
    shared = game_app_shared_resources();
    render_full_scene(phase, (phase == FISHING_PHASE_BITE), reel_line_x);
    draw_hud(phase,
             status_line,
             (shared != 0) ? shared->food : 0u,
             (shared != 0) ? shared->relic : 0u);
  } else if (need_partial) {
    // Only update the reel line; bar borders are untouched because
    // the line is drawn strictly inside the bar area.
    erase_reel_line(reel_prev_line_x);
    draw_reel_line(reel_line_x);
  }
}
