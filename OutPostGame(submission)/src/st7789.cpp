#include "st7789.h"
#include "config.h"
#include <SPI.h>

SPIClass spiLCD(LCD_MOSI, PB14, LCD_SCK);
ST7789 lcd;

static inline void swSpiWrite(uint8_t data) {
    spiLCD.transfer(data);
}

static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x00,0x00,0x5F,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00}, // "
    {0x14,0x7F,0x14,0x7F,0x14}, // #
    {0x24,0x2A,0x7F,0x2A,0x12}, // $
    {0x23,0x13,0x08,0x64,0x62}, // %
    {0x36,0x49,0x55,0x22,0x50}, // &
    {0x00,0x05,0x03,0x00,0x00}, // '
    {0x00,0x1C,0x22,0x41,0x00}, // (
    {0x00,0x41,0x22,0x1C,0x00}, // )
    {0x14,0x08,0x3E,0x08,0x14}, // *
    {0x08,0x08,0x3E,0x08,0x08}, // +
    {0x00,0x50,0x30,0x00,0x00}, // ,
    {0x08,0x08,0x08,0x08,0x08}, // -
    {0x00,0x60,0x60,0x00,0x00}, // .
    {0x20,0x10,0x08,0x04,0x02}, // /
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9
    {0x00,0x36,0x36,0x00,0x00}, // :
    {0x00,0x56,0x36,0x00,0x00}, // ;
    {0x08,0x14,0x22,0x41,0x00}, // <
    {0x14,0x14,0x14,0x14,0x14}, // =
    {0x00,0x41,0x22,0x14,0x08}, // >
    {0x02,0x01,0x51,0x09,0x06}, // ?
    {0x32,0x49,0x79,0x41,0x3E}, // @
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x3F,0x40,0x38,0x40,0x3F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x07,0x08,0x70,0x08,0x07}, // Y
    {0x61,0x51,0x49,0x45,0x43}, // Z
    {0x00,0x7F,0x41,0x41,0x00}, // [
    {0x02,0x04,0x08,0x10,0x20}, // \
    {0x00,0x41,0x41,0x7F,0x00}, // ]
    {0x04,0x02,0x01,0x02,0x04}, // ^
    {0x40,0x40,0x40,0x40,0x40}, // _
    {0x00,0x01,0x02,0x04,0x00}, // `
    {0x20,0x54,0x54,0x54,0x78}, // a
    {0x7F,0x48,0x44,0x44,0x38}, // b
    {0x38,0x44,0x44,0x44,0x20}, // c
    {0x38,0x44,0x44,0x48,0x7F}, // d
    {0x38,0x54,0x54,0x54,0x18}, // e
    {0x08,0x7E,0x09,0x01,0x02}, // f
    {0x08,0x14,0x54,0x54,0x3C}, // g
    {0x7F,0x08,0x04,0x04,0x78}, // h
    {0x00,0x44,0x7D,0x40,0x00}, // i
    {0x20,0x40,0x44,0x3D,0x00}, // j
    {0x7F,0x10,0x28,0x44,0x00}, // k
    {0x00,0x41,0x7F,0x40,0x00}, // l
    {0x7C,0x04,0x18,0x04,0x78}, // m
    {0x7C,0x08,0x04,0x04,0x78}, // n
    {0x38,0x44,0x44,0x44,0x38}, // o
    {0x7C,0x14,0x14,0x14,0x08}, // p
    {0x08,0x14,0x14,0x18,0x7C}, // q
    {0x7C,0x08,0x04,0x04,0x08}, // r
    {0x48,0x54,0x54,0x54,0x20}, // s
    {0x04,0x3F,0x44,0x40,0x20}, // t
    {0x3C,0x40,0x40,0x20,0x7C}, // u
    {0x1C,0x20,0x40,0x20,0x1C}, // v
    {0x3C,0x40,0x30,0x40,0x3C}, // w
    {0x44,0x28,0x10,0x28,0x44}, // x
    {0x0C,0x50,0x50,0x50,0x3C}, // y
    {0x44,0x64,0x54,0x4C,0x44}, // z
    {0x00,0x08,0x36,0x41,0x00}, // {
    {0x00,0x00,0x7F,0x00,0x00}, // |
    {0x00,0x41,0x36,0x08,0x00}, // }
    {0x08,0x04,0x08,0x10,0x08}, // ~
};

void ST7789::writeCommand(uint8_t cmd) {
    digitalWrite(LCD_DC, LOW);
    digitalWrite(LCD_CS, LOW);
    swSpiWrite(cmd);
    digitalWrite(LCD_CS, HIGH);
}

void ST7789::writeData(uint8_t data) {
    digitalWrite(LCD_DC, HIGH);
    digitalWrite(LCD_CS, LOW);
    swSpiWrite(data);
    digitalWrite(LCD_CS, HIGH);
}

void ST7789::writeDataBuffer(const uint8_t* data, size_t len) {
    digitalWrite(LCD_DC, HIGH);
    digitalWrite(LCD_CS, LOW);
    for (size_t i = 0; i < len; i++) {
        swSpiWrite(data[i]);
    }
    digitalWrite(LCD_CS, HIGH);
}

void ST7789::reset() {
    digitalWrite(LCD_RST, HIGH);
    delay(20);
    digitalWrite(LCD_RST, LOW);
    delay(20);
    digitalWrite(LCD_RST, HIGH);
    delay(120);
}

void ST7789::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    x0 += LCD_X_OFFSET;
    x1 += LCD_X_OFFSET;
    y0 += LCD_Y_OFFSET;
    y1 += LCD_Y_OFFSET;

    writeCommand(0x2A);
    writeData(x0 >> 8);
    writeData(x0 & 0xFF);
    writeData(x1 >> 8);
    writeData(x1 & 0xFF);

    writeCommand(0x2B);
    writeData(y0 >> 8);
    writeData(y0 & 0xFF);
    writeData(y1 >> 8);
    writeData(y1 & 0xFF);
}

void ST7789::begin() {
    pinMode(LCD_CS, OUTPUT);
    pinMode(LCD_DC, OUTPUT);
    pinMode(LCD_RST, OUTPUT);
    pinMode(LCD_BL, OUTPUT);

    digitalWrite(LCD_CS, HIGH);
    digitalWrite(LCD_DC, HIGH);
    digitalWrite(LCD_BL, HIGH);

    spiLCD.begin();
    spiLCD.setClockDivider(SPI_CLOCK_DIV4);

    digitalWrite(LCD_RST, LOW);
    delay(50);
    digitalWrite(LCD_RST, HIGH);
    delay(150);

    writeCommand(0x01); delay(150);
    writeCommand(0x11); delay(120);

    writeCommand(0x3A); writeData(0x55); delay(10);
    writeCommand(0x36); writeData(0x00); delay(10);

    writeCommand(0xB2); writeData(0x0C); writeData(0x0C); writeData(0x00); writeData(0x33); writeData(0x33);
    writeCommand(0xB7); writeData(0x35);
    writeCommand(0xBB); writeData(0x19);
    writeCommand(0xC0); writeData(0x2C);
    writeCommand(0xC2); writeData(0x01);
    writeCommand(0xC3); writeData(0x12);
    writeCommand(0xC4); writeData(0x20);
    writeCommand(0xC6); writeData(0x0F);
    writeCommand(0xD0); writeData(0xA4); writeData(0xA1);

    writeCommand(0x21); delay(10);
    writeCommand(0x13); delay(10);
    writeCommand(0x29); delay(150);
}

void ST7789::setRotation(uint8_t r) {
    writeCommand(0x36);
    switch (r & 3) {
        case 0: writeData(0x00); break;
        case 1: writeData(0x60); break;
        case 2: writeData(0xC0); break;
        case 3: writeData(0xA0); break;
    }
}

void ST7789::setRenderTarget(uint16_t* buffer, int16_t x, int16_t y, int16_t w, int16_t h) {
    rtBuf = buffer;
    rtX = x;
    rtY = y;
    rtW = w;
    rtH = h;
}

void ST7789::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (rtBuf) {
        if (x >= rtX && x < rtX + rtW && y >= rtY && y < rtY + rtH) {
            rtBuf[(y - rtY) * rtW + (x - rtX)] = color;
        }
        return;
    }

    if (x < 0 || y < 0 || x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    setAddrWindow(x, y, x, y);
    digitalWrite(LCD_DC, LOW);
    digitalWrite(LCD_CS, LOW);
    swSpiWrite(0x2C);
    digitalWrite(LCD_DC, HIGH);
    swSpiWrite(color >> 8);
    swSpiWrite(color & 0xFF);
    digitalWrite(LCD_CS, HIGH);
}

void ST7789::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    fillRect(x, y, w, 1, color);
}

void ST7789::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    fillRect(x, y, 1, h, color);
}

void ST7789::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (rtBuf) {
        int16_t bx = x, by = y, bw = w, bh = h;
        if (bx < rtX) { bw -= rtX - bx; bx = rtX; }
        if (by < rtY) { bh -= rtY - by; by = rtY; }
        if (bx + bw > rtX + rtW) bw = rtX + rtW - bx;
        if (by + bh > rtY + rtH) bh = rtY + rtH - by;
        if (bw > 0 && bh > 0) {
            for (int16_t j = 0; j < bh; j++) {
                int16_t idx = (by + j - rtY) * rtW + (bx - rtX);
                for (int16_t i = 0; i < bw; i++) rtBuf[idx + i] = color;
            }
        }
        return;
    }

    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + w <= 0 || y + h <= 0) return;

    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    setAddrWindow(x, y, x + w - 1, y + h - 1);
    digitalWrite(LCD_DC, LOW);
    digitalWrite(LCD_CS, LOW);
    swSpiWrite(0x2C);
    digitalWrite(LCD_DC, HIGH);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    for (int32_t i = 0; i < (int32_t)w * h; i++) {
        swSpiWrite(hi);
        swSpiWrite(lo);
    }
    digitalWrite(LCD_CS, HIGH);
}

void ST7789::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y + h - 1, w, color);
    drawFastVLine(x, y, h, color);
    drawFastVLine(x + w - 1, y, h, color);
}

void ST7789::pushImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) {
    if (x < 0 || y < 0 || x + w > LCD_WIDTH || y + h > LCD_HEIGHT) return;

    setAddrWindow(x, y, x + w - 1, y + h - 1);
    digitalWrite(LCD_DC, LOW);
    digitalWrite(LCD_CS, LOW);
    swSpiWrite(0x2C);
    digitalWrite(LCD_DC, HIGH);

    for (int32_t i = 0; i < (int32_t)w * h; i++) {
        uint16_t c = data[i];
        swSpiWrite(c >> 8);
        swSpiWrite(c & 0xFF);
    }
    digitalWrite(LCD_CS, HIGH);
}

void ST7789::drawBitmapTransparent(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data, uint16_t transparentColor) {
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            uint16_t c = data[j * w + i];
            if (c != transparentColor) drawPixel(x + i, y + j, c);
        }
    }
}

void ST7789::drawSubBitmapTransparent(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data, int16_t src_w, uint16_t transparentColor) {
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            uint16_t c = data[j * src_w + i];
            if (c != transparentColor) drawPixel(x + i, y + j, c);
        }
    }
}

void ST7789::fillScreen(uint16_t color) {
    fillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

void ST7789::drawChar5x7(int16_t x, int16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t* bmp = font5x7[c - 32];

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = bmp[col];
        for (uint8_t row = 0; row < 8; row++) {
            uint16_t color = (line & 1) ? fg : bg;
            if (scale == 1) drawPixel(x + col, y + row, color);
            else fillRect(x + col * scale, y + row * scale, scale, scale, color);
            line >>= 1;
        }
    }

    if (scale == 1) fillRect(x + 5, y, 1, 8, bg);
    else fillRect(x + 5 * scale, y, scale, 8 * scale, bg);
}

void ST7789::drawText(int16_t x, int16_t y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale) {
    while (*text) {
        drawChar5x7(x, y, *text, fg, bg, scale);
        x += 6 * scale;
        text++;
    }
}