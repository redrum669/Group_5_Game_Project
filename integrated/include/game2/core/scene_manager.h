#ifndef GAME_CORE_SCENE_MANAGER_H
#define GAME_CORE_SCENE_MANAGER_H

#include <stdbool.h>

#include "game2/core/scene_id.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  game_scene_id_t current;
  game_scene_id_t pending;
  bool has_pending;
} game_scene_manager_t;

void game_scene_manager_init(game_scene_manager_t* manager);
void game_scene_manager_request(game_scene_manager_t* manager, game_scene_id_t next);
void game_scene_manager_apply_pending(game_scene_manager_t* manager);
game_scene_id_t game_scene_manager_current(const game_scene_manager_t* manager);
bool game_scene_manager_is_transitioning(const game_scene_manager_t* manager);

#ifdef __cplusplus
}
#endif

#endif
