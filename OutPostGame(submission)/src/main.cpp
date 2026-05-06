#include <Arduino.h>
#include "st7789.h"
#include "game.h"
#include "input.h"
#include "config.h"

// System initialization (runs once at startup)
void setup() {
    // Boot indicator: blink LED 5 times to confirm the MCU is running
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
    }

    // Initialize display, input, and game system
    lcd.begin();
    lcd.setRotation(0);
    inputInit();
    gameInit();

    // Start serial port for debug and sound control
    Serial.begin(115200);
    delay(50);
}

// Main loop (runs continuously)
void loop() {
    uint32_t frameStart = millis();

    // LED heartbeat: shows the system is alive
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

    // Run game logic and rendering
    gameUpdate();
    gameRender();

    // Maintain stable ~60 FPS (16ms per frame)
    uint32_t frameTime = millis() - frameStart;
    if (frameTime < 16) {
        delay(16 - frameTime);
    }
}