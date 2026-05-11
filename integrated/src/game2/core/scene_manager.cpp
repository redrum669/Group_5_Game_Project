#include "game2/core/scene_manager.h"

void game_scene_manager_init(game_scene_manager_t* manager) {
  if (manager == 0) {
    return;
  }

  manager->current = GAME_SCENE_CONSOLE;
  manager->pending = GAME_SCENE_CONSOLE;
  manager->has_pending = false;
}

void game_scene_manager_request(game_scene_manager_t* manager, game_scene_id_t next) {
  if (manager == 0) {
    return;
  }

  manager->pending = next;
  manager->has_pending = true;
}

void game_scene_manager_apply_pending(game_scene_manager_t* manager) {
  if (manager == 0 || !manager->has_pending) {
    return;
  }

  manager->current = manager->pending;
  manager->has_pending = false;
}

game_scene_id_t game_scene_manager_current(const game_scene_manager_t* manager) {
  if (manager == 0) {
    return GAME_SCENE_CONSOLE;
  }

  return manager->current;
}

bool game_scene_manager_is_transitioning(const game_scene_manager_t* manager) {
  if (manager == 0) {
    return false;
  }

  return manager->has_pending;
}
