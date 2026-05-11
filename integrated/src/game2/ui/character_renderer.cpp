#include "game2/ui/character_renderer.h"

#include "game2/config/reference/reference_style.h"
#include "game2/drivers/lcd_driver.h"

#define CHARACTER_SIZE GAME_REF_CHARACTER_BASE_SIZE
#define CHARACTER_SCALE GAME_REF_CHARACTER_SCALE

static uint8_t s_char_frame = 0;
static game_character_dir_t s_char_dir = GAME_CHARACTER_DIR_DOWN;

static const uint8_t s_armor_layer[2][8] = {
    {0x3C, 0x7E, 0x42, 0x7E, 0x81, 0x81, 0x00, 0x66},
    {0x3C, 0x7E, 0x42, 0x7E, 0x81, 0x81, 0x00, 0xC3}};

static const uint8_t s_suit_layer[2][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x7E, 0x7E, 0x66, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x7E, 0x7E, 0x66, 0x00}};

static const uint8_t s_visor_layer[8] = {0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00};

static void game_character_renderer_draw_layer(int16_t px,
                                               int16_t py,
                                               const uint8_t* data,
                                               uint16_t color) {
  int row;
  int col;

  for (row = 0; row < CHARACTER_SIZE; row++) {
    uint8_t row_data = data[row];
    for (col = 0; col < CHARACTER_SIZE; col++) {
      if (((row_data >> (7 - col)) & 0x01u) != 0u) {
        game_lcd_fill_rect((int16_t)(px + col * CHARACTER_SCALE),
                           (int16_t)(py + row * CHARACTER_SCALE),
                           CHARACTER_SCALE,
                           CHARACTER_SCALE,
                           color);
      }
    }
  }
}

void game_character_renderer_init(void) {
  s_char_dir = GAME_CHARACTER_DIR_DOWN;
  s_char_frame = 0;
}

void game_character_renderer_set_dir(game_character_dir_t dir) {
  s_char_dir = dir;
  (void)s_char_dir;
  s_char_frame ^= 1u;
}

void game_character_renderer_draw(int16_t px, int16_t py) {
  game_character_renderer_draw_layer(px,
                                     py,
                                     s_suit_layer[s_char_frame],
                                     GAME_REF_CHAR_SUIT_COLOR);
  game_character_renderer_draw_layer(px,
                                     py,
                                     s_armor_layer[s_char_frame],
                                     GAME_REF_CHAR_ARMOR_COLOR);
  game_character_renderer_draw_layer(px, py, s_visor_layer, GAME_REF_CHAR_VISOR_COLOR);
}
