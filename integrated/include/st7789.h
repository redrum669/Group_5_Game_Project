#ifndef ST7789_H
#define ST7789_H

#include <Arduino.h>

class ST7789 {
public:
    void begin();
    void setRotation(uint8_t r);
    void fillScreen(uint16_t color);
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void pushImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data);
    void drawBitmapTransparent(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data, uint16_t transparentColor = 0xF81F);
    void drawSubBitmapTransparent(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data, int16_t src_w, uint16_t transparentColor = 0xF81F);
    void setRenderTarget(uint16_t* buffer, int16_t x, int16_t y, int16_t w, int16_t h);

    void drawChar5x7(int16_t x, int16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale = 1);
    void drawText(int16_t x, int16_t y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale = 1);

private:
    uint16_t* rtBuf = nullptr;
    int16_t rtX = 0, rtY = 0, rtW = 0, rtH = 0;

    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void writeDataBuffer(const uint8_t* data, size_t len);
    void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void reset();
};

extern ST7789 lcd;

#endif
