#include <Arduino.h>
#include "unified_app.h"

void setup() {
    unified_app_init();
}

void loop() {
    unified_app_tick();
}
