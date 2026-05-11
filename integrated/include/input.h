#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>

// ── Unified input init / update ──────────────────────────────────────────────
void inputInit();
void updateInput();

// ── Exploration-style accessors ──────────────────────────────────────────────
bool isUpPressed();
bool isDownPressed();
bool isLeftPressed();
bool isRightPressed();

bool isButtonAPressed();
bool isButtonBPressed();
bool isButtonCPressed();
bool isButtonDPressed();
bool isJoySWPressed();

// ── Farming-style accessors ──────────────────────────────────────────────────
// These are provided so the farming module can link without changes.

#ifdef __cplusplus
extern "C" {
#endif

void game_input_update(void);
bool game_input_up(void);
bool game_input_down(void);
bool game_input_left(void);
bool game_input_right(void);
bool game_input_button_a_edge(void);
bool game_input_button_b_edge(void);
bool game_input_button_c_edge(void);
bool game_input_button_d_edge(void);
bool game_input_cw_pulse(void);
uint32_t game_platform_millis(void);

#ifdef __cplusplus
}
#endif

#endif
