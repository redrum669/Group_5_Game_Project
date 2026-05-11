#ifndef SOUND_H
#define SOUND_H

#include <Arduino.h>
#include <stdint.h>

void soundInit();
void playWeaponFire(int weaponType);
void updateChargingSound(bool isCharging, uint32_t chargeStartMs);
void playJumpSound();
void playWeaponSwapSound();
void playLaserRiseSound();
void updateLaserRiseSound(uint32_t elapsedMs);
void playTransmissionLogMusic();
void stopTransmissionLogMusic();
void soundUpdate();

#endif
