#include "game1/systems/sound.h"
#include "config.h"
#include "game1/entities/weapon.h"

void soundInit() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void playWeaponFire(int weaponType) {
    switch (weaponType) {
        case WEAPON_SMG:
            tone(BUZZER_PIN, 450, 25);
            break;
        case WEAPON_MG:
            tone(BUZZER_PIN, 320, 50);
            break;
        case WEAPON_LASER:
            tone(BUZZER_PIN, 700, 120);
            break;
        case WEAPON_LASER_GUN:
            tone(BUZZER_PIN, 900, 180);
            break;
    }
}

void updateChargingSound(bool isCharging, uint32_t chargeStartMs) {
    static uint32_t lastSoundUpdate = 0;

    if (isCharging) {
        uint32_t now = millis();
        if (now - lastSoundUpdate > 30) { // Update frequency smoothly
            uint32_t elapsed = now - chargeStartMs;
            if (elapsed > 1000) elapsed = 1000;

            int freq = 200 + elapsed;
            tone(BUZZER_PIN, freq);
            lastSoundUpdate = now;
        }
    } else {
        noTone(BUZZER_PIN);
    }
}

void playJumpSound() {
    tone(BUZZER_PIN, 600, 80);
}

void playWeaponSwapSound() {
    tone(BUZZER_PIN, 800, 40);
}

void playLaserRiseSound() {
    // This could just be a start trigger, but updateLaserRiseSound is better.
}

void updateLaserRiseSound(uint32_t elapsedMs) {
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();

    if (now - lastUpdate < 30) return;
    lastUpdate = now;

    if (elapsedMs >= 1000 && elapsedMs < 3200) {
        int freq = 1500 - (500 * (elapsedMs - 1000) / 2200);
        freq += (now % 200) - 100;
        tone(BUZZER_PIN, freq);
    } else if (elapsedMs >= 3200 && elapsedMs < 6200) {
        int freq = 400 + (600 * (elapsedMs - 3200) / 3000);
        tone(BUZZER_PIN, freq);
    } else if (elapsedMs >= 6200) {
        noTone(BUZZER_PIN);
    }
}

void soundUpdate() {
    // No additional buzzer update needed for single buzzer logic.
}

void playTransmissionLogMusic() {
    Serial.println("MUSIC:PLAY_EPILOGUE");
}

void stopTransmissionLogMusic() {
    Serial.println("MUSIC:STOP_EPILOGUE");
}
