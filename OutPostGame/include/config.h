#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =========================
// LCD Pins
// =========================
#define LCD_CS    PB12
#define LCD_DC    PB11
#define LCD_RST   PB2
#define LCD_BL    PB1
#define LCD_SCK   PB13
#define LCD_MOSI  PB15

// =========================
// LCD Size
// =========================
#define LCD_WIDTH   240
#define LCD_HEIGHT  240

#define LCD_X_OFFSET 0
#define LCD_Y_OFFSET 0

// =========================
// UI Layout
// =========================
#define STATUS_BAR_H 24
#define ACTION_BAR_H 40

#define PLAY_AREA_X 0
#define PLAY_AREA_Y 24
#define PLAY_AREA_W 240
#define PLAY_AREA_H 176

#define TILE_SIZE 16
#define MAP_COLS 60
#define MAP_ROWS 11
#define VISIBLE_COLS (PLAY_AREA_W / TILE_SIZE)

// Physics constants for platformer mode (Combat Areas)
#define GRAVITY_PX 1
#define JUMP_FORCE_PX 10

// =========================
// Colors (RGB565)
// =========================
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_ORANGE      0xFD20
#define COLOR_GRAY        0x8410
#define COLOR_DARKGRAY    0x4208
#define COLOR_LIGHTGRAY   0xC618
#define COLOR_BROWN       0xA145
#define COLOR_BG_PANEL    0x2128
#define COLOR_BG_WORLD    0x39E7
#define COLOR_HP          0xF800
#define COLOR_HUNGER      0xFD20
#define COLOR_ARMOR       0x7D7C
#define COLOR_DARKGREEN   0x03E0
#define COLOR_GOLD        0xFDA0
#define LED_BUILTIN PA5

// =========================
// Joystick Pins (Analog)
// =========================
#define JOY_X     A2
#define JOY_Y     A1

// Joystick center / deadzone
#define JOY_CENTER    512
#define JOY_DEADZONE  150

// =========================
// Buttons
// =========================
#define BTN_A   PA8
#define BTN_B   PA9
#define BTN_C   PA10
#define BTN_D   PA2

// =========================
// Joystick SW (center press)
// =========================
#define JOY_SW  A5

// =========================
// Buzzer / Speaker
// =========================
#define BUZZER_PIN PA6

// =========================
// Survival System
// =========================
#define HUNGER_DRAIN_INTERVAL  15000
#define STARVATION_INTERVAL     5000
#define STARVATION_DAMAGE          3
#define FOOD_HUNGER_RESTORE       30
#define ARMOR_RESTORE             30
#define MEDKIT_HEAL               30

#endif
