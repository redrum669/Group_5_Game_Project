#include "sound.h"
#include "config.h"
#include "weapon.h"

#ifdef BUZZER2_PIN
static HardwareTimer *buzzer2Timer = nullptr;
static uint32_t buzzer2Channel = 0;
static uint32_t buzzer2StopTime = 0;
#endif

void soundInit() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

#ifdef BUZZER2_PIN
    pinMode(BUZZER2_PIN, OUTPUT);
    digitalWrite(BUZZER2_PIN, LOW);

    // Initialize hardware PWM timer for second buzzer
    PinName pin = digitalPinToPinName(BUZZER2_PIN);
    TIM_TypeDef *Instance = (TIM_TypeDef *)pinmap_peripheral(pin, PinMap_PWM);

    if (Instance) {
        buzzer2Channel = STM_PIN_CHANNEL(pinmap_function(pin, PinMap_PWM));
        buzzer2Timer = new HardwareTimer(Instance);
        buzzer2Timer->setMode(buzzer2Channel, TIMER_OUTPUT_COMPARE_PWM1, BUZZER2_PIN);
        buzzer2Timer->pause();
    }
#endif
}

void playWeaponFire(int weaponType) {
    switch (weaponType) {
        case WEAPON_SMG:
            tone(BUZZER_PIN, 200, 30);
            break;
        case WEAPON_MG:
            tone(BUZZER_PIN, 120, 60);
            break;
        case WEAPON_LASER:
            tone(BUZZER_PIN, 90, 150);
            break;
        case WEAPON_LASER_GUN:
            tone(BUZZER_PIN, 80, 250);
            break;
        default:
            break;
    }
}

void updateChargingSound(bool isCharging, uint32_t chargeStartMs) {
    static uint32_t lastUpdate = 0;

    if (isCharging) {
        uint32_t now = millis();
        if (now - lastUpdate > 30) {
            uint32_t elapsed = now - chargeStartMs;
            if (elapsed > 1000) elapsed = 1000;

            int freq = 200 + (1000 * elapsed / 1000);

#ifdef BUZZER2_PIN
            if (buzzer2Timer) {
                buzzer2Timer->setOverflow(freq, HERTZ_FORMAT);
                buzzer2Timer->setCaptureCompare(buzzer2Channel, 50, PERCENT_COMPARE_FORMAT);
                buzzer2Timer->resume();
                buzzer2StopTime = 0;
            } else {
                tone(BUZZER2_PIN, freq);
            }
#else
            tone(BUZZER_PIN, freq);
#endif
            lastUpdate = now;
        }
    } else {
#ifdef BUZZER2_PIN
        if (buzzer2Timer) {
            buzzer2Timer->pause();
            digitalWrite(BUZZER2_PIN, LOW);
        } else {
            noTone(BUZZER2_PIN);
        }
#else
        noTone(BUZZER_PIN);
#endif
    }
}

void playJumpSound() {
#ifdef BUZZER2_PIN
    if (buzzer2Timer) {
        buzzer2Timer->setOverflow(600, HERTZ_FORMAT);
        buzzer2Timer->setCaptureCompare(buzzer2Channel, 50, PERCENT_COMPARE_FORMAT);
        buzzer2Timer->resume();
        buzzer2StopTime = millis() + 80;
    } else {
        tone(BUZZER2_PIN, 600, 80);
    }
#endif
}

void playWeaponSwapSound() {
#ifdef BUZZER2_PIN
    if (buzzer2Timer) {
        buzzer2Timer->setOverflow(800, HERTZ_FORMAT);
        buzzer2Timer->setCaptureCompare(buzzer2Channel, 50, PERCENT_COMPARE_FORMAT);
        buzzer2Timer->resume();
        buzzer2StopTime = millis() + 40;
    } else {
        tone(BUZZER2_PIN, 800, 40);
    }
#endif
}

void updateLaserRiseSound(uint32_t elapsedMs) {
#ifdef BUZZER2_PIN
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();

    if (now - lastUpdate < 30) return;
    lastUpdate = now;

    if (elapsedMs >= 1000 && elapsedMs < 3200) {
        int freq = 1500 - (500 * (elapsedMs - 1000) / 2200);
        freq += (now % 200) - 100;

        if (buzzer2Timer) {
            buzzer2Timer->setOverflow(freq, HERTZ_FORMAT);
            buzzer2Timer->setCaptureCompare(buzzer2Channel, 50, PERCENT_COMPARE_FORMAT);
            buzzer2Timer->resume();
            buzzer2StopTime = 0;
        } else {
            tone(BUZZER2_PIN, freq);
        }
    }
    else if (elapsedMs >= 3200 && elapsedMs < 6200) {
        int freq = 400 + (600 * (elapsedMs - 3200) / 3000);

        if (buzzer2Timer) {
            buzzer2Timer->setOverflow(freq, HERTZ_FORMAT);
            buzzer2Timer->setCaptureCompare(buzzer2Channel, 50, PERCENT_COMPARE_FORMAT);
            buzzer2Timer->resume();
            buzzer2StopTime = 0;
        } else {
            tone(BUZZER2_PIN, freq);
        }
    }
    else if (elapsedMs >= 6200) {
        if (buzzer2Timer) {
            buzzer2Timer->pause();
            digitalWrite(BUZZER2_PIN, LOW);
        } else {
            noTone(BUZZER2_PIN);
        }
    }
#endif
}

void soundUpdate() {
#ifdef BUZZER2_PIN
    if (buzzer2StopTime > 0 && millis() >= buzzer2StopTime) {
        buzzer2StopTime = 0;
        if (buzzer2Timer) {
            buzzer2Timer->pause();
            digitalWrite(BUZZER2_PIN, LOW);
        } else {
            noTone(BUZZER2_PIN);
        }
    }
#endif
}

void playTransmissionLogMusic() {
    Serial.println("MUSIC:PLAY_EPILOGUE");
}

void stopTransmissionLogMusic() {
    Serial.println("MUSIC:STOP_EPILOGUE");
}