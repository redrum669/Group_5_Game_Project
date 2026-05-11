#include "input.h"
#include "config.h"

// We use a robust Exponential Moving Average (EMA) to filter out hardware noise and spikes.
static int smoothedX = 512;
static int smoothedY = 512;
static int centerX = 512;
static int centerY = 512;

static bool upPressed    = false;
static bool downPressed  = false;
static bool leftPressed  = false;
static bool rightPressed = false;

static bool btnAPressed  = false;
static bool btnBPressed  = false;
static bool btnCPressed  = false;
static bool btnDPressed  = false;
static bool joySwPressed = false;

// ── inputInit ─────────────────────────────────────────────────────────────────
void inputInit() {
    analogReadResolution(10); // 0 - 1023

    pinMode(BTN_A,  INPUT_PULLUP);
    pinMode(BTN_B,  INPUT_PULLUP);
    pinMode(BTN_C,  INPUT_PULLUP);
    pinMode(BTN_D,  INPUT_PULLUP);
    pinMode(JOY_SW, INPUT_PULLUP);

    // Calibrate center over a few reads to prevent hardware drift
    long sumX = 0, sumY = 0;
    for (int i = 0; i < 16; i++) {
        sumX += analogRead(JOY_X);
        sumY += analogRead(JOY_Y);
        delay(5);
    }
    centerX = sumX / 16;
    centerY = sumY / 16;
    smoothedX = centerX;
    smoothedY = centerY;
}

// ── updateInput ───────────────────────────────────────────────────────────────
void updateInput() {
    int rawX = analogRead(JOY_X);
    int rawY = analogRead(JOY_Y);

    // Filter: 75% old value, 25% new value. Smooths out sudden jumps or electrical noise.
    smoothedX = (smoothedX * 3 + rawX) / 4;
    smoothedY = (smoothedY * 3 + rawY) / 4;

    // Use relative offsets from the calibrated center
    int dx = smoothedX - centerX;
    int dy = smoothedY - centerY;

    // Extremely strict deadzone: MUST move at least 300 units away from center
    // Only deliberate, full pushes will register. Completely ignores drift.
    leftPressed  = (dx < -300);
    rightPressed = (dx > 300);
    upPressed    = (dy < -300);
    downPressed  = (dy > 300);

    btnAPressed  = (digitalRead(BTN_A)  == LOW);
    btnBPressed  = (digitalRead(BTN_B)  == LOW);
    btnCPressed  = (digitalRead(BTN_C)  == LOW);
    btnDPressed  = (digitalRead(BTN_D)  == LOW);
    joySwPressed = (digitalRead(JOY_SW) == LOW);
}

// ── Accessors ─────────────────────────────────────────────────────────────────
bool isUpPressed()    { return upPressed; }
bool isDownPressed()  { return downPressed; }
bool isLeftPressed()  { return leftPressed; }
bool isRightPressed() { return rightPressed; }

bool isButtonAPressed() { return btnAPressed; }
bool isButtonBPressed() { return btnBPressed; }
bool isButtonCPressed() { return btnCPressed; }
bool isButtonDPressed() { return btnDPressed; }
bool isJoySWPressed()   { return joySwPressed; }
