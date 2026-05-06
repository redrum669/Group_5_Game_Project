#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include <Arduino.h>

void initAudioSystem(void);
void playFireSound(int weaponType);
void playHitSound(void);
void playPickupSound(void);
void updateChargeSound(bool charging, uint32_t startTime);
void stopAllSounds(void);

#endif
