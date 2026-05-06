#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include <Arduino.h>

void setupInputPins(void);
void sampleInputState(void);

bool upKeyPressed(void);
bool downKeyPressed(void);
bool leftKeyPressed(void);
bool rightKeyPressed(void);

bool btnAPressed(void);
bool btnBPressed(void);
bool btnCPressed(void);
bool btnDPressed(void);
bool joystickBtnPressed(void);

#endif