#ifndef ST7789_DISPLAY_H
#define ST7789_DISPLAY_H

#include <Arduino.h>
#include <SPI.h>

class ST7789_Display {
public:
  ST7789_Display(int8_t dc, int8_t cs, int8_t rst);
  void initPanel(void);
  void clearScreen(uint16_t color);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void drawText(int16_t x, int16_t y, const char* str, uint16_t fg, uint16_t bg, uint8_t size);
  void drawImage(int16_t x, int16_t y, const uint16_t* img);
};

extern ST7789_Display lcd;

#endif