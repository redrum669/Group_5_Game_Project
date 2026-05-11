#ifndef GAME_UI_CHARACTER_RENDERER_H
#define GAME_UI_CHARACTER_RENDERER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  GAME_CHARACTER_DIR_UP = 0,
  GAME_CHARACTER_DIR_DOWN,
  GAME_CHARACTER_DIR_LEFT,
  GAME_CHARACTER_DIR_RIGHT
} game_character_dir_t;

void game_character_renderer_init(void);
void game_character_renderer_set_dir(game_character_dir_t dir);
void game_character_renderer_draw(int16_t px, int16_t py);

#ifdef __cplusplus
}
#endif

#endif
