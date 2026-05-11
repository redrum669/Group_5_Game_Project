#ifndef GAME_APP_GAME_APP_H
#define GAME_APP_GAME_APP_H

#include "game2/core/game_state.h"
#include "game2/core/scene_manager.h"
#include "game2/data/shared_resources.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  game_state_t state;
  game_scene_manager_t scene_manager;
  game_shared_resources_t shared;
} game_app_t;

void game_app_init(game_app_t* app);
void game_app_tick(game_app_t* app);

void game_app_request_scene(game_scene_id_t next);
game_shared_resources_t* game_app_shared_resources(void);

#ifdef __cplusplus
}
#endif

#endif
