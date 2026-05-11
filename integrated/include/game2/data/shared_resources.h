#ifndef GAME_DATA_SHARED_RESOURCES_H
#define GAME_DATA_SHARED_RESOURCES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FARM_TILE_COUNT 9

typedef struct {
  uint16_t food;
  uint16_t metal;
  uint16_t day;
  uint16_t player_hp;
  uint8_t tool_level;
  uint16_t relic;
  uint8_t farm_tile_state[FARM_TILE_COUNT];
  uint32_t farm_tile_since[FARM_TILE_COUNT];
  uint8_t weapon_unlocked_mask;
} game_shared_resources_t;

#ifdef __cplusplus
}
#endif

#endif
