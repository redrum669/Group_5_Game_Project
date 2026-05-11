#include "input.h"
#include "config.h"

// ── Joystick state ───────────────────────────────────────────────────────────
static int smoothedX = 2048;
static int smoothedY = 2048;
static int centerX   = 2048;
static int centerY   = 2048;

static bool upPressed    = false;
static bool downPressed  = false;
static bool leftPressed  = false;
static bool rightPressed = false;

// ── Button state (current frame) ─────────────────────────────────────────────
static bool btnAPressed  = false;
static bool btnBPressed  = false;
static bool btnCPressed  = false;
static bool btnDPressed  = false;
static bool joySwPressed = false;

// ── Button edge detection ────────────────────────────────────────────────────
static bool prevBtnA = false;
static bool prevBtnB = false;
static bool prevBtnC = false;
static bool prevBtnD = false;
static bool edgeBtnA = false;
static bool edgeBtnB = false;
static bool edgeBtnC = false;
static bool edgeBtnD = false;

// ── Clockwise rotation detection (farming fishing mini-game) ─────────────────
static int s_last_sector = -1;
static uint8_t s_cw_pulse_count = 0;
static uint8_t s_deadzone_frames = 0;

static int joystick_sector(int dx, int dy) {
    if (dx > 0 && dy <= 0) return (-dy > dx) ? 7 : 0;
    if (dx > 0 && dy > 0)  return (dy > dx)  ? 2 : 1;
    if (dx <= 0 && dy > 0) return (dy > -dx) ? 3 : 4;
    return (-dy > -dx) ? 6 : 5;
}

// ── Initialization ───────────────────────────────────────────────────────────
void inputInit() {
    analogReadResolution(12); // 0 - 4095

    pinMode(BTN_A,  INPUT_PULLUP);
    pinMode(BTN_B,  INPUT_PULLUP);
    pinMode(BTN_C,  INPUT_PULLUP);
    pinMode(BTN_D,  INPUT_PULLUP);
    pinMode(JOY_SW, INPUT_PULLUP);

    long sumX = 0, sumY = 0;
    for (int i = 0; i < 16; i++) {
        sumX += analogRead(JOY_X);
        sumY += analogRead(JOY_Y);
        delay(5);
    }
    centerX = sumX / 16;
    centerY = sumY / 16;
    if (centerX < 300 || centerX > 3800) centerX = 2048;
    if (centerY < 300 || centerY > 3800) centerY = 2048;
    smoothedX = centerX;
    smoothedY = centerY;
}

// ── Per-frame update ─────────────────────────────────────────────────────────
void updateInput() {
    int rawX = analogRead(JOY_X);
    int rawY = analogRead(JOY_Y);

    // EMA filter: 75% old + 25% new
    smoothedX = (smoothedX * 3 + rawX) / 4;
    smoothedY = (smoothedY * 3 + rawY) / 4;

    int dx = smoothedX - centerX;
    int dy = smoothedY - centerY;

    // Directional digital output (large deadzone for deliberate pushes)
    leftPressed  = (dx < -JOY_DEADZONE);
    rightPressed = (dx >  JOY_DEADZONE);
    upPressed    = (dy < -JOY_DEADZONE);
    downPressed  = (dy >  JOY_DEADZONE);

    // Button polling
    bool curA = (digitalRead(BTN_A) == LOW);
    bool curB = (digitalRead(BTN_B) == LOW);
    bool curC = (digitalRead(BTN_C) == LOW);
    bool curD = (digitalRead(BTN_D) == LOW);

    btnAPressed  = curA;
    btnBPressed  = curB;
    btnCPressed  = curC;
    btnDPressed  = curD;
    joySwPressed = (digitalRead(JOY_SW) == LOW);

    // Edge detection (pressed this frame, not last frame)
    edgeBtnA = curA && !prevBtnA;
    edgeBtnB = curB && !prevBtnB;
    edgeBtnC = curC && !prevBtnC;
    edgeBtnD = curD && !prevBtnD;
    prevBtnA = curA;
    prevBtnB = curB;
    prevBtnC = curC;
    prevBtnD = curD;

    // Clockwise rotation detection for farming mini-games
    {
        int dist_sq = dx * dx + dy * dy;
        int deadzone_sq = JOY_DEADZONE * JOY_DEADZONE;
        if (dist_sq > deadzone_sq) {
            int sector = joystick_sector(dx, dy);
            if (s_last_sector >= 0) {
                int diff = (sector - s_last_sector + 8) % 8;
                if (diff == 1 || diff == 2) {
                    if (s_cw_pulse_count < 255) s_cw_pulse_count++;
                }
            }
            s_last_sector = sector;
            s_deadzone_frames = 0;
        } else {
            s_deadzone_frames++;
            if (s_deadzone_frames > 2) s_last_sector = -1;
        }
    }
}

// ── Exploration-style accessors ──────────────────────────────────────────────
bool isUpPressed()    { return upPressed; }
bool isDownPressed()  { return downPressed; }
bool isLeftPressed()  { return leftPressed; }
bool isRightPressed() { return rightPressed; }

bool isButtonAPressed() { return btnAPressed; }
bool isButtonBPressed() { return btnBPressed; }
bool isButtonCPressed() { return btnCPressed; }
bool isButtonDPressed() { return btnDPressed; }
bool isJoySWPressed()   { return joySwPressed; }

// ── Farming-style accessors ──────────────────────────────────────────────────
extern "C" {

void game_input_update(void) {
    // Farming expects to call this, but in the unified project
    // updateInput() is already called once per frame by the host.
    // Calling it again is harmless (just re-reads ADC one extra time).
    updateInput();
}

bool game_input_up(void)    { return upPressed; }
bool game_input_down(void)  { return downPressed; }
bool game_input_left(void)  { return leftPressed; }
bool game_input_right(void) { return rightPressed; }
bool game_input_button_a_edge(void) { return edgeBtnA; }
bool game_input_button_b_edge(void) { return edgeBtnB; }
bool game_input_button_c_edge(void) { return edgeBtnC; }
bool game_input_button_d_edge(void) { return edgeBtnD; }

bool game_input_cw_pulse(void) {
    if (s_cw_pulse_count > 0) {
        s_cw_pulse_count--;
        return true;
    }
    return false;
}

bool game_input_button_a_pressed(void) { return btnAPressed; }
bool game_input_button_b_pressed(void) { return btnBPressed; }

uint32_t game_platform_millis(void) { return millis(); }

} // extern "C"
