#include <Arduino.h>
#include "st7789.h"
#include "game.h"
#include "input.h"
#include "config.h"

/**
 * @brief System initialization function, runs once at startup
 */
void setup() {
    // Boot diagnostic: blink LD2 (PA5) 5 times to confirm MCU is running.
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
    }

    // Initialize the LCD display driver
    lcd.begin();
    // Set LCD display rotation to 0 degrees (default orientation)
    lcd.setRotation(0);
    // Initialize input hardware and buttons
    inputInit();
    // Initialize game state and variables
    gameInit();
    // Initialize serial communication with baud rate 115200
    Serial.begin(115200);
    // Give Serial a tiny bit of time to initialize
    delay(50);
}

/**
 * @brief Main program loop, runs continuously
 */
void loop() {
    uint32_t frameStart = millis();

    // Heartbeat: flip LD2 every frame so a steady flicker proves the loop is alive.
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

    // Update game logic, input handling and state
    gameUpdate();
    // Render all graphics to the LCD display
    gameRender();

    // Delay to maintain approximately 60 FPS frame rate (16ms per frame)
    uint32_t frameTime = millis() - frameStart;
    if (frameTime < 16) {
        delay(16 - frameTime);
    }
}