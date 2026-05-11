#include "game2/app/game_app.h"

#include "game2/scenes/cg_scene.h"
#include "game2/scenes/console_scene.h"
#include "game2/scenes/farm_scene.h"
#include "game2/scenes/fishing_scene.h"
#include "game2/scenes/forge_scene.h"
#include "game2/scenes/home_scene.h"
#include "game2/scenes/world_scene.h"
#include "game2/drivers/platform_backend.h"

static game_app_t* s_active_app = 0;
static game_scene_id_t s_last_scene = GAME_SCENE_CONSOLE;

static void game_app_update_console(game_app_t* app) {
  (void)app;
  game_console_scene_update();
}

static void game_app_update_cg(game_app_t* app) {
  (void)app;
  game_cg_scene_update();
}

static void game_app_update_world(game_app_t* app) {
  (void)app;
  game_world_scene_update();
}

static void game_app_update_farm(game_app_t* app) {
  (void)app;
  game_farm_scene_update();
}

static void game_app_update_fishing(game_app_t* app) {
  (void)app;
  game_fishing_scene_update();
}

static void game_app_update_forge(game_app_t* app) {
  (void)app;
  game_forge_scene_update();
}

static void game_app_update_home(game_app_t* app) {
  (void)app;
  game_home_scene_update();
}

void game_app_init(game_app_t* app) {
  if (app == 0) {
    return;
  }

  app->shared.food = 10;
  app->shared.metal = 5;
  app->shared.day = 1;
  app->shared.player_hp = 100;
  app->shared.tool_level = 1;
  app->shared.relic = 0;
  app->shared.weapon_unlocked_mask = 0;
  {
    int i;
    for (i = 0; i < FARM_TILE_COUNT; i++) {
      app->shared.farm_tile_state[i] = 0;
      app->shared.farm_tile_since[i] = 0;
    }
  }

  game_platform_backend_init();
  game_scene_manager_init(&app->scene_manager);
  app->state = GAME_STATE_RUNNING;
  s_active_app = app;
  s_last_scene = GAME_SCENE_CONSOLE;
}

void game_app_request_scene(game_scene_id_t next) {
  if (s_active_app == 0) {
    return;
  }

  game_scene_manager_request(&s_active_app->scene_manager, next);
}

game_shared_resources_t* game_app_shared_resources(void) {
  if (s_active_app == 0) {
    return 0;
  }

  return &s_active_app->shared;
}

void game_app_tick(game_app_t* app) {
  game_scene_id_t scene;

  if (app == 0 || app->state != GAME_STATE_RUNNING) {
    return;
  }

  game_scene_manager_apply_pending(&app->scene_manager);
  scene = game_scene_manager_current(&app->scene_manager);

  if (scene != s_last_scene) {
    if (scene == GAME_SCENE_WORLD) {
      game_world_scene_request_full_redraw();
    } else if (scene == GAME_SCENE_FARM) {
      game_farm_scene_request_full_redraw();
    } else if (scene == GAME_SCENE_FISHING) {
      game_fishing_scene_request_full_redraw();
    } else if (scene == GAME_SCENE_HOME) {
      game_home_scene_request_full_redraw();
    } else if (scene == GAME_SCENE_FORGE) {
      game_forge_scene_request_full_redraw();
    } else if (scene == GAME_SCENE_CONSOLE) {
      game_console_scene_request_full_redraw();
    } else if (scene == GAME_SCENE_CG) {
      game_cg_scene_request_full_redraw();
    }
    s_last_scene = scene;
  }

  switch (scene) {
    case GAME_SCENE_WORLD:
      game_app_update_world(app);
      break;
    case GAME_SCENE_FARM:
      game_app_update_farm(app);
      break;
    case GAME_SCENE_FISHING:
      game_app_update_fishing(app);
      break;
    case GAME_SCENE_FORGE:
      game_app_update_forge(app);
      break;
    case GAME_SCENE_HOME:
      game_app_update_home(app);
      break;
    case GAME_SCENE_CONSOLE:
      game_app_update_console(app);
      break;
    case GAME_SCENE_CG:
      game_app_update_cg(app);
      break;
    default:
      break;
  }
}
