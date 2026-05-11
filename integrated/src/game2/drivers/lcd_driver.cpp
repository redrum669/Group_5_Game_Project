#include "game2/drivers/lcd_driver.h"

static const game_lcd_backend_t* s_backend = 0;

void game_lcd_bind_backend(const game_lcd_backend_t* backend) {
  s_backend = backend;
}

void game_lcd_init(void) {
  // Phase1 baseline: backend binding is optional.
  // If no backend is bound yet, calls become safe no-ops.
}

bool game_lcd_is_ready(void) {
  return s_backend != 0;
}

void game_lcd_fill_screen(uint16_t color) {
  if (s_backend != 0 && s_backend->fill_screen != 0) {
    s_backend->fill_screen(color);
  }
}

void game_lcd_draw_pixel(int16_t x, int16_t y, uint16_t color) {
  if (s_backend != 0 && s_backend->draw_pixel != 0) {
    s_backend->draw_pixel(x, y, color);
  }
}

void game_lcd_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if (s_backend != 0 && s_backend->fill_rect != 0) {
    s_backend->fill_rect(x, y, w, h, color);
  }
}

void game_lcd_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if (s_backend != 0 && s_backend->draw_rect != 0) {
    s_backend->draw_rect(x, y, w, h, color);
  }
}

void game_lcd_draw_text(int16_t x,
                        int16_t y,
                        const char* text,
                        uint16_t fg,
                        uint16_t bg,
                        uint8_t scale) {
  if (s_backend != 0 && s_backend->draw_text != 0) {
    s_backend->draw_text(x, y, text, fg, bg, scale);
  }
}
