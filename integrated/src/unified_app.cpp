#include "unified_app.h"
#include "config.h"
#include "input.h"
#include "st7789.h"
#include "game1/core/game.h"
#include "game1/scenes/console.h"

extern "C" {
#include "game2/app/game_app.h"
}

enum UnifiedMode {
    MODE_CONSOLE,
    MODE_GAME_1,
    MODE_GAME_2
};

static UnifiedMode s_mode = MODE_CONSOLE;
static bool s_consoleDirty = true;
static game_app_t s_game2_app;

// Direct GPIO debounce for the global "return to host" key (BTN_D).
// We do NOT use the shared input system here so that each game mode
// can own its own input polling without double-update side effects.
static bool s_prevRawD = false;

void unified_app_init() {
    // ── Hardware ─────────────────────────────────────────────────────────────
    lcd.begin();
    lcd.setRotation(0);
    inputInit();
    Serial.begin(115200);
    delay(50);

    pinMode(LED_HOST, OUTPUT);

    // ── Game 1 (Exploration) ─────────────────────────────────────────────────
    gameInit();
    gameSetStartState(1);          // skip boot / CG, start at world map
    gameClearReturnRequest();

    // ── Game 2 (Farming) ─────────────────────────────────────────────────────
    game_app_init(&s_game2_app);
    game_app_request_scene(GAME_SCENE_WORLD);
}

void unified_app_tick() {
    uint32_t frameStart = millis();

    // Heartbeat LED
    digitalWrite(LED_HOST, !digitalRead(LED_HOST));

    // ── Global "return to host console" key ──────────────────────────────────
    bool rawD = (digitalRead(BTN_D) == LOW);
    bool dEdge = rawD && !s_prevRawD;
    s_prevRawD = rawD;

    if (dEdge && s_mode != MODE_CONSOLE) {
        s_mode = MODE_CONSOLE;
        s_consoleDirty = true;
        gameClearReturnRequest();
        // Ensure farming restarts at world map next time
        game_app_request_scene(GAME_SCENE_WORLD);
        return;
    }

    switch (s_mode) {
        // ── Host Console ─────────────────────────────────────────────────────
        case MODE_CONSOLE: {
            // Console mode drives input centrally
            updateInput();

            if (s_consoleDirty) {
                lcd.fillScreen(COLOR_BLACK);
                consoleDrawMainMenu();
                s_consoleDirty = false;
            }

            if (game_input_button_a_edge()) {
                s_mode = MODE_GAME_1;
                s_consoleDirty = true;
                gameSetStartState(1);
                gameClearReturnRequest();
            }
            else if (game_input_button_b_edge()) {
                s_mode = MODE_GAME_2;
                s_consoleDirty = true;
                game_app_request_scene(GAME_SCENE_WORLD);
            }
            break;
        }

        // ── Game 1 : Exploration ─────────────────────────────────────────────
        case MODE_GAME_1: {
            gameUpdate();
            gameRender();
            if (gameIsRequestingReturnToHost()) {
                s_mode = MODE_CONSOLE;
                s_consoleDirty = true;
                gameClearReturnRequest();
            }
            break;
        }

        // ── Game 2 : Farming ─────────────────────────────────────────────────
        case MODE_GAME_2: {
            game_app_tick(&s_game2_app);
            break;
        }
    }

    // ~60 FPS frame pacing
    uint32_t frameTime = millis() - frameStart;
    if (frameTime < 16) {
        delay(16 - frameTime);
    }
}
