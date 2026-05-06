#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <Arduino.h>

// hardware pin definitions
#define LCD_PIN_DC         10
#define LCD_PIN_CS        11
#define LCD_PIN_RST        6
#define JOYSTICK_X_PIN    A1
#define JOYSTICK_Y_PIN    A0
#define BUTTON_A_PIN       7
#define BUTTON_B_PIN       8
#define BUZZER_PIN         5

// display and tile settings
#define SCREEN_WIDTH       240
#define SCREEN_HEIGHT      240
#define TILE_PIXEL_SIZE    16
#define MAP_GRID_ROWS      15
#define MAP_GRID_COLS      15

// game balance values
#define PLAYER_MAX_HP      100
#define PLAYER_MAX_HUNGER  100
#define DEFAULT_INV_VALUE   0
#define ENEMY_SPAWN_LIMIT   8

// color definitions (RGB565)
#define COLOR_BLACK        0x0000
#define COLOR_WHITE        0xFFFF
#define COLOR_RED          0xF800
#define COLOR_GREEN        0x07E0
#define COLOR_BLUE         0x001F
#define COLOR_GREY         0x8410

#endif