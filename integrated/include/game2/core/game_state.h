#ifndef GAME_CORE_GAME_STATE_H
#define GAME_CORE_GAME_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  GAME_STATE_BOOT = 0,
  GAME_STATE_RUNNING,
  GAME_STATE_PAUSED,
  GAME_STATE_TRANSITION
} game_state_t;

#ifdef __cplusplus
}
#endif

#endif
