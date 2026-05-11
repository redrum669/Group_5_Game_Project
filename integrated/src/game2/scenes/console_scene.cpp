#include "game2/scenes/console_scene.h"

#include <stdbool.h>
#include <stdint.h>

#include "game2/app/game_app.h"
#include "game2/config/reference/reference_style.h"
#include "game2/core/scene_id.h"
#include "game2/drivers/lcd_driver.h"
#include "game2/drivers/platform_backend.h"
#include "game2/ui/character_renderer.h"

// ---------------------------------------------------------------------------
// Color mapping from reference console.cpp
// ---------------------------------------------------------------------------
#define CONSOLE_BG        0x0847
#define CONSOLE_BORDER1   0x39CE
#define CONSOLE_BORDER2   0x632C
#define CONSOLE_CARD_BG   0x2108
#define CONSOLE_CARD_IN1  0x296B
#define CONSOLE_CARD_IN2  0x426D
#define CONSOLE_SKY1      0x18C8
#define CONSOLE_SKY2      0x31AA
#define CONSOLE_SKY3      0x4A8E
#define CONSOLE_SKY4      0x73CF
#define CONSOLE_GROUND    0x4221
#define CONSOLE_GROUND2   0x8A84
#define CONSOLE_HORIZON   0x6ACA
#define CONSOLE_ROCK      0x52AE
#define CONSOLE_ROCK2     0x4208
#define CONSOLE_ROCK3     0x528A
#define CONSOLE_GRASS     0x07E0
#define CONSOLE_GRASS2    0x17E0
#define CONSOLE_GREEN2    0x03E0
#define CONSOLE_GREEN3    0x05A0
#define CONSOLE_GREEN4    0x62E0
#define CONSOLE_HAY       0xC638
#define CONSOLE_HAYTOP    0xA2A0
#define CONSOLE_HAYDARK   0x6B4D
#define CONSOLE_HAYDET    0x7D9F
#define CONSOLE_BTN_BG    0x18C6
#define CONSOLE_GOLD      0x8400
#define CONSOLE_STAR      0x632C

// ---------------------------------------------------------------------------
// Drawing helpers (adapted from reference console.cpp)
// ---------------------------------------------------------------------------
static void draw_menu_sprite_layer(int16_t px, int16_t py, const uint8_t* data, uint16_t color) {
  int row;
  for (row = 0; row < 8; row++) {
    uint8_t rowBits = data[row];
    int col;
    for (col = 0; col < 8; col++) {
      if ((rowBits >> (7 - col)) & 1) {
        game_lcd_fill_rect((int16_t)(px + col * 2), (int16_t)(py + row * 2), 2, 2, color);
      }
    }
  }
}

static void draw_menu_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
  int16_t x = 0;
  int16_t y = r;
  int16_t d = (int16_t)(3 - 2 * r);
  while (x <= y) {
    game_lcd_draw_pixel((int16_t)(cx + x), (int16_t)(cy + y), color);
    game_lcd_draw_pixel((int16_t)(cx - x), (int16_t)(cy + y), color);
    game_lcd_draw_pixel((int16_t)(cx + x), (int16_t)(cy - y), color);
    game_lcd_draw_pixel((int16_t)(cx - x), (int16_t)(cy - y), color);
    game_lcd_draw_pixel((int16_t)(cx + y), (int16_t)(cy + x), color);
    game_lcd_draw_pixel((int16_t)(cx - y), (int16_t)(cy + x), color);
    game_lcd_draw_pixel((int16_t)(cx + y), (int16_t)(cy - x), color);
    game_lcd_draw_pixel((int16_t)(cx - y), (int16_t)(cy - x), color);
    if (d < 0) {
      d += (int16_t)(4 * x + 6);
    } else {
      d += (int16_t)(4 * (x - y) + 10);
      y--;
    }
    x++;
  }
}

static void draw_cow_sprite(int16_t px, int16_t py, bool facingLeft) {
  static const uint8_t body[8] = {
    0x00, 0x7E, 0xFF, 0xFF, 0xFF, 0x7E, 0x24, 0x24
  };
  static const uint8_t spotA[8] = {
    0x00, 0x00, 0x10, 0x18, 0x08, 0x10, 0x00, 0x00
  };
  static const uint8_t spotB[8] = {
    0x00, 0x00, 0x06, 0x0E, 0x04, 0x08, 0x00, 0x00
  };
  static const uint8_t headLeft[8] = {
    0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xE0, 0xC0, 0x00
  };
  static const uint8_t headRight[8] = {
    0x00, 0x00, 0x03, 0x07, 0x0F, 0x07, 0x03, 0x00
  };
  static const uint8_t hornLeft[8] = {
    0x00, 0x80, 0x40, 0x00, 0x80, 0x00, 0x00, 0x00
  };
  static const uint8_t hornRight[8] = {
    0x00, 0x01, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00
  };

  draw_menu_sprite_layer(px, py, body, GAME_REF_COLOR_WHITE);
  draw_menu_sprite_layer(px, py, spotA, GAME_REF_COLOR_BLACK);
  draw_menu_sprite_layer(px, py, spotB, GAME_REF_COLOR_BLACK);
  if (facingLeft) {
    draw_menu_sprite_layer(px, py, hornLeft, 0xC618);
    draw_menu_sprite_layer(px, py, headLeft, GAME_REF_COLOR_WHITE);
    game_lcd_draw_pixel((int16_t)(px + 1), (int16_t)(py + 8), GAME_REF_COLOR_BLACK);
  } else {
    draw_menu_sprite_layer(px, py, hornRight, 0xC618);
    draw_menu_sprite_layer(px, py, headRight, GAME_REF_COLOR_WHITE);
    game_lcd_draw_pixel((int16_t)(px + 14), (int16_t)(py + 8), GAME_REF_COLOR_BLACK);
  }
}

static void draw_menu_border(void) {
  game_lcd_fill_screen(CONSOLE_BG);
  game_lcd_draw_rect(6, 6, 228, 228, CONSOLE_BORDER1);
  game_lcd_draw_rect(8, 8, 224, 224, CONSOLE_BORDER2);

  int i;
  for (i = 0; i < 8; i++) {
    int16_t x = (int16_t)(18 + (i * 23) % 190);
    int16_t y = (int16_t)(92 + (i * 13) % 18);
    game_lcd_fill_rect(x, y, 2, 2, CONSOLE_STAR);
  }
}

static void draw_menu_ship(int16_t x, int16_t y) {
  draw_menu_circle(x, y, 10, GAME_REF_COLOR_YELLOW);
  draw_menu_circle(x, y, 11, CONSOLE_GOLD);
  game_lcd_fill_rect((int16_t)(x - 14), (int16_t)(y - 3), 28, 8, 0x4208);
  game_lcd_fill_rect((int16_t)(x - 9),  (int16_t)(y - 6), 18, 5, 0x8410);
  game_lcd_fill_rect((int16_t)(x - 4),  (int16_t)(y - 9),  8, 4, 0xC618);
  game_lcd_fill_rect((int16_t)(x - 4),  (int16_t)(y - 6),  8, 5, GAME_REF_COLOR_CYAN);
  draw_menu_circle(x, (int16_t)(y - 4), 5, GAME_REF_COLOR_WHITE);
  game_lcd_fill_rect((int16_t)(x - 11), (int16_t)(y + 3), 3, 2, 0xFD20);
  game_lcd_fill_rect((int16_t)(x - 3),  (int16_t)(y + 3), 3, 2, 0xFD20);
  game_lcd_fill_rect((int16_t)(x + 4),  (int16_t)(y + 3), 3, 2, 0xFD20);
  game_lcd_fill_rect((int16_t)(x + 9),  (int16_t)(y + 3), 3, 2, 0xFD20);
}

static void draw_alien_creature(int16_t px, int16_t py) {
  static const uint8_t body[8]  = {0x00, 0x3C, 0x7E, 0xFF, 0xBD, 0xFF, 0x66, 0x42};
  static const uint8_t eyes[8]  = {0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00};
  static const uint8_t claws[8] = {0x00, 0x00, 0x00, 0x81, 0x42, 0x81, 0x00, 0x00};

  draw_menu_sprite_layer(px, py, body,  0x7BE0);
  draw_menu_sprite_layer(px, py, eyes,  0xF81F);
  draw_menu_sprite_layer(px, py, claws, GAME_REF_COLOR_WHITE);
}

static void draw_exploration_card(int16_t x, int16_t y) {
  game_lcd_fill_rect(x, y, 88, 118, CONSOLE_CARD_BG);
  game_lcd_draw_rect(x, y, 88, 118, GAME_REF_COLOR_YELLOW);
  game_lcd_draw_rect((int16_t)(x + 1), (int16_t)(y + 1), 86, 116, GAME_REF_COLOR_YELLOW);

  game_lcd_fill_rect((int16_t)(x + 4), (int16_t)(y + 4), 80, 86, CONSOLE_CARD_IN1);
  int py;
  for (py = 0; py < 36; py++) {
    uint16_t c = (py < 18) ? CONSOLE_SKY1 : CONSOLE_SKY2;
    game_lcd_fill_rect((int16_t)(x + 4), (int16_t)(y + 4 + py), 80, 1, c);
  }
  draw_menu_ship((int16_t)(x + 61), (int16_t)(y + 16));
  game_lcd_fill_rect((int16_t)(x + 58), (int16_t)(y + 22), 6, 18, CONSOLE_GRASS);
  game_lcd_fill_rect((int16_t)(x + 57), (int16_t)(y + 39), 8, 6, CONSOLE_GRASS2);
  game_lcd_fill_rect((int16_t)(x + 14), (int16_t)(y + 48), 10, 16, CONSOLE_ROCK);
  game_lcd_fill_rect((int16_t)(x + 24), (int16_t)(y + 42), 8, 22, CONSOLE_ROCK2);
  game_lcd_fill_rect((int16_t)(x + 62), (int16_t)(y + 50), 7, 16, CONSOLE_ROCK2);
  game_lcd_fill_rect((int16_t)(x + 4), (int16_t)(y + 64), 80, 1, CONSOLE_HORIZON);
  game_lcd_fill_rect((int16_t)(x + 4), (int16_t)(y + 66), 80, 20, CONSOLE_GROUND);
  game_lcd_fill_rect((int16_t)(x + 8), (int16_t)(y + 74), 18, 6, CONSOLE_ROCK3);
  game_lcd_fill_rect((int16_t)(x + 68), (int16_t)(y + 72), 10, 8, CONSOLE_ROCK3);

  game_character_renderer_draw((int16_t)(x + 23), (int16_t)(y + 48));
  draw_alien_creature((int16_t)(x + 52), (int16_t)(y + 58));

  game_lcd_fill_rect((int16_t)(x + 11), (int16_t)(y + 94), 66, 14, CONSOLE_BTN_BG);
  game_lcd_draw_rect((int16_t)(x + 11), (int16_t)(y + 94), 66, 14, GAME_REF_COLOR_YELLOW);
  game_lcd_draw_text((int16_t)(x + 15), (int16_t)(y + 98), "Exploration",
                     GAME_REF_COLOR_WHITE, CONSOLE_BTN_BG, 1);
}

static void draw_farming_card(int16_t x, int16_t y) {
  game_lcd_fill_rect(x, y, 88, 118, CONSOLE_CARD_BG);
  game_lcd_draw_rect(x, y, 88, 118, GAME_REF_COLOR_YELLOW);
  game_lcd_draw_rect((int16_t)(x + 1), (int16_t)(y + 1), 86, 116, GAME_REF_COLOR_YELLOW);

  game_lcd_fill_rect((int16_t)(x + 4), (int16_t)(y + 4), 80, 86, CONSOLE_CARD_IN2);
  int py;
  for (py = 0; py < 36; py++) {
    uint16_t c = (py < 16) ? CONSOLE_SKY3 : CONSOLE_SKY4;
    game_lcd_fill_rect((int16_t)(x + 4), (int16_t)(y + 4 + py), 80, 1, c);
  }
  game_lcd_fill_rect((int16_t)(x + 4), (int16_t)(y + 62), 80, 24, CONSOLE_GROUND2);
  int row;
  for (row = 0; row < 3; row++) {
    game_lcd_fill_rect((int16_t)(x + 4), (int16_t)(y + 66 + row * 7), 80, 1, 0x6A03);
  }
  int i;
  for (i = 0; i < 6; i++) {
    int16_t cx = (int16_t)(x + 10 + i * 12);
    game_lcd_fill_rect(cx, (int16_t)(y + 64 + (i & 1)), 1, 18, CONSOLE_GREEN2);
    game_lcd_fill_rect((int16_t)(cx + 2), (int16_t)(y + 66 + ((i + 1) & 1)), 1, 14, CONSOLE_GRASS);
    game_lcd_draw_pixel((int16_t)(cx - 1), (int16_t)(y + 80), CONSOLE_GREEN3);
  }
  game_lcd_fill_rect((int16_t)(x + 56), (int16_t)(y + 50), 18, 14, CONSOLE_HAY);
  game_lcd_fill_rect((int16_t)(x + 62), (int16_t)(y + 43), 1, 6, CONSOLE_HAYTOP);
  game_lcd_fill_rect((int16_t)(x + 60), (int16_t)(y + 44), 1, 10, CONSOLE_HAYTOP);
  game_lcd_fill_rect((int16_t)(x + 58), (int16_t)(y + 45), 1, 14, CONSOLE_HAYTOP);
  game_lcd_fill_rect((int16_t)(x + 56), (int16_t)(y + 46), 1, 18, CONSOLE_HAYTOP);
  game_lcd_fill_rect((int16_t)(x + 55), (int16_t)(y + 47), 1, 20, CONSOLE_HAYTOP);
  game_lcd_fill_rect((int16_t)(x + 55), (int16_t)(y + 48), 1, 20, CONSOLE_HAYTOP);
  game_lcd_fill_rect((int16_t)(x + 55), (int16_t)(y + 49), 1, 20, CONSOLE_HAYTOP);
  game_lcd_fill_rect((int16_t)(x + 64), (int16_t)(y + 56), 1, 8, CONSOLE_GREEN4);
  game_lcd_fill_rect((int16_t)(x + 58), (int16_t)(y + 54), 4, 4, CONSOLE_HAYDET);
  game_lcd_fill_rect((int16_t)(x + 68), (int16_t)(y + 54), 4, 4, CONSOLE_HAYDET);
  game_lcd_draw_rect((int16_t)(x + 56), (int16_t)(y + 50), 18, 14, CONSOLE_HAYDARK);

  draw_cow_sprite((int16_t)(x + 10), (int16_t)(y + 46), false);
  game_character_renderer_draw((int16_t)(x + 35), (int16_t)(y + 41));

  game_lcd_fill_rect((int16_t)(x + 17), (int16_t)(y + 94), 54, 14, CONSOLE_BTN_BG);
  game_lcd_draw_rect((int16_t)(x + 17), (int16_t)(y + 94), 54, 14, GAME_REF_COLOR_YELLOW);
  game_lcd_draw_text((int16_t)(x + 24), (int16_t)(y + 98), "Farming",
                     GAME_REF_COLOR_WHITE, CONSOLE_BTN_BG, 1);
}

static void draw_main_menu(void) {
  game_character_renderer_init();
  draw_menu_border();

  game_lcd_draw_text(48, 28, "OUTPOST OF THE LAST DAWN",
                     GAME_REF_COLOR_WHITE, CONSOLE_BG, 1);
  game_lcd_fill_rect(32, 50, 176, 1, CONSOLE_BORDER2);

  draw_exploration_card(20, 94);
  draw_farming_card(132, 94);

  game_lcd_draw_text(22, 214, "A: START EXPLORATION",
                     GAME_REF_COLOR_YELLOW, CONSOLE_BG, 1);
  game_lcd_draw_text(67, 226, "B: FARMING",
                     0x07E0, CONSOLE_BG, 1);
}

// ---------------------------------------------------------------------------
// Scene update
// ---------------------------------------------------------------------------
static bool s_console_force_redraw = false;

void game_console_scene_request_full_redraw(void) {
  s_console_force_redraw = true;
}

void game_console_scene_update(void) {
  static bool initialized = false;

  if (s_console_force_redraw) {
    initialized = false;
    s_console_force_redraw = false;
  }

  if (!initialized) {
    game_lcd_init();
    draw_main_menu();
    initialized = true;
  }

  game_input_update();

  if (game_input_button_a_edge()) {
    initialized = false;
    game_app_request_scene(GAME_SCENE_WORLD);
    return;
  }

  if (game_input_button_b_edge()) {
    initialized = false;
    game_app_request_scene(GAME_SCENE_CG);
    return;
  }
}
