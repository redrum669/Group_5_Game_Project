#ifndef GAME_DRIVERS_PLATFORM_BACKEND_H
#define GAME_DRIVERS_PLATFORM_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void game_platform_backend_init(void);
void game_led_init(void);
void game_led_set(bool on);

void game_buzzer_init(void);
void game_buzzer_tone(uint16_t freq, uint16_t duration_ms);
void game_buzzer_stop(void);

void game_input_update(void);
bool game_input_up(void);
bool game_input_down(void);
bool game_input_left(void);
bool game_input_right(void);
bool game_input_button_a_edge(void);
bool game_input_button_b_edge(void);
bool game_input_button_a_pressed(void);
bool game_input_button_b_pressed(void);
bool game_input_cw_pulse(void);
uint32_t game_platform_millis(void);

#ifdef __cplusplus
}
#endif

#endif
