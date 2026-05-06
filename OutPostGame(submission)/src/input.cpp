#include "input.h"
#include "config.h"

// EMA filtering for joystick noise reduction
static int smoothX = 512;
static int smoothY = 512;
static int joyCenterX = 512;
static int joyCenterY = 512;

static bool up    = false;
static bool down  = false;
static bool left  = false;
static bool right = false;

static bool btnA = false;
static bool btnB = false;
static bool btnC = false;
static bool btnD = false;
static bool joySw = false;

void inputInit() {
    analogReadResolution(10); // 0~1023

    pinMode(BTN_A,  INPUT_PULLUP);
    pinMode(BTN_B,  INPUT_PULLUP);
    pinMode(BTN_C,  INPUT_PULLUP);
    pinMode(BTN_D,  INPUT_PULLUP);
    pinMode(JOY_SW, INPUT_PULLUP);

    // Auto‑calibrate joystick center
    long sumX = 0, sumY = 0;
    for (int i = 0; i < 16; i++) {
        sumX += analogRead(JOY_X);
        sumY += analogRead(JOY_Y);
        delay(5);
    }
    joyCenterX = sumX / 16;
    joyCenterY = sumY / 16;
    smoothX = joyCenterX;
    smoothY = joyCenterY;
}

void updateInput() {
    int rawX = analogRead(JOY_X);
    int rawY = analogRead(JOY_Y);

    // Smooth filter: 75% old, 25% new
    smoothX = (smoothX * 3 + rawX) / 4;
    smoothY = (smoothY * 3 + rawY) / 4;

    int dx = smoothX - joyCenterX;
    int dy = smoothY - joyCenterY;

    // Large deadzone to avoid drift
    left  = (dx < -300);
    right = (dx > 300);
    up    = (dy < -300);
    down  = (dy > 300);

    btnA = (digitalRead(BTN_A)  == LOW);
    btnB = (digitalRead(BTN_B)  == LOW);
    btnC = (digitalRead(BTN_C)  == LOW);
    btnD = (digitalRead(BTN_D)  == LOW);
    joySw = (digitalRead(JOY_SW) == LOW);
}

bool isUpPressed()    { return up;    }
bool isDownPressed()  { return down;  }
bool isLeftPressed()  { return left;  }
bool isRightPressed() { return right; }

bool isButtonAPressed() { return btnA; }
bool isButtonBPressed() { return btnB; }
bool isButtonCPressed() { return btnC; }
bool isButtonDPressed() { return btnD; }
bool isJoySWPressed()   { return joySw; }