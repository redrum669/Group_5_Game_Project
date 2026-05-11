#ifndef GAME_CORE_SCENE_ID_H
#define GAME_CORE_SCENE_ID_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  GAME_SCENE_CONSOLE = 0,
  GAME_SCENE_WORLD,
  GAME_SCENE_FARM,
  GAME_SCENE_FISHING,
  GAME_SCENE_FORGE,
  GAME_SCENE_HOME,
  GAME_SCENE_CG
} game_scene_id_t;

#ifdef __cplusplus
}
#endif

#endif
