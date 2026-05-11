#ifndef GAME_DRIVERS_LCD_DRIVER_H
#define GAME_DRIVERS_LCD_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  void (*fill_screen)(uint16_t color);
  void (*draw_pixel)(int16_t x, int16_t y, uint16_t color);
  void (*fill_rect)(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void (*draw_rect)(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void (*draw_text)(int16_t x, int16_t y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale);
} game_lcd_backend_t;

void game_lcd_bind_backend(const game_lcd_backend_t* backend);
void game_lcd_init(void);
bool game_lcd_is_ready(void);

void game_lcd_fill_screen(uint16_t color);
void game_lcd_draw_pixel(int16_t x, int16_t y, uint16_t color);
void game_lcd_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void game_lcd_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void game_lcd_draw_text(int16_t x,
                        int16_t y,
                        const char* text,
                        uint16_t fg,
                        uint16_t bg,
                        uint8_t scale);

#ifdef __cplusplus
}
#endif

#endif
