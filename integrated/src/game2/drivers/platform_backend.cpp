#include <Arduino.h>

extern "C" {
#include "game2/drivers/lcd_driver.h"
#include "game2/drivers/platform_backend.h"
}

#include "config.h"
#include "st7789.h"

// LCD backend bridges to the unified ST7789 instance
static void backend_fill_screen(uint16_t color) { lcd.fillScreen(color); }
static void backend_draw_pixel(int16_t x, int16_t y, uint16_t color) { lcd.drawPixel(x, y, color); }
static void backend_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    lcd.fillRect(x, y, w, h, color);
}
static void backend_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    lcd.drawRect(x, y, w, h, color);
}
static void backend_draw_text(int16_t x, int16_t y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale) {
    lcd.drawText(x, y, text, fg, bg, scale);
}

void game_platform_backend_init(void) {
    static game_lcd_backend_t backend = {
        backend_fill_screen, backend_draw_pixel, backend_fill_rect, backend_draw_rect, backend_draw_text
    };
    game_lcd_bind_backend(&backend);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void game_led_init(void) {}

void game_led_set(bool on) {
    digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void game_buzzer_init(void) {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void game_buzzer_tone(uint16_t freq, uint16_t duration_ms) {
    if (freq > 0) {
        tone(BUZZER_PIN, freq, duration_ms);
    } else {
        noTone(BUZZER_PIN);
    }
}

void game_buzzer_stop(void) {
    noTone(BUZZER_PIN);
}
