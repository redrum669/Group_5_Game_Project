#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>

void inputInit();
void updateInput();

bool isUpPressed();
bool isDownPressed();
bool isLeftPressed();
bool isRightPressed();

bool isButtonAPressed();
bool isButtonBPressed();
bool isButtonCPressed();
bool isButtonDPressed();  // portal interact (PA2)
bool isJoySWPressed();   // joystick center button

#endif
