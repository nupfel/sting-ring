#include <Arduino.h>
#include <EEPROM.h>
#include "main.h"

Poofer poofer[NUM_POOFERS] = {
        {2, true},
        {3, false},
        {4, false},
        {5, false},
        {6, false},
        {7, true}
};

const uint8_t button[NUM_POOFERS] = {8, 9, 10, 11, 12, 13};
unsigned short int mode = DEFAULT_MODE;
bool record = false;
bool off = false;
uint32_t last_button_press = 0;
bool record_start = false;
uint16_t eeAddr = 0;

// List of patterns to cycle through.  Each is defined as a separate function below.
SimplePatternList gPatterns = { pattern1 };
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current

/*
    SETUP
 */
void setup() {
        serial_setup();
        pin_setup();
}

/*
    LOOP
 */
void loop() {
        if (off) return;

        handleModeRecord();
        switch (mode) {
        case RECORD_MODE:
                if (record) {
                        recordPattern();
                        handleButtons();
                }
                else {
                        playPattern();
                }
                break;
        case SIMON_MODE:
                simon();
                break;
        default:
                handleButtons();
                if (millis() - last_button_press > IDLE_TIMEOUT) {
                        idle_pattern();
                }
        }

}

/*
    SETUP functions
 */
void pin_setup() {
        pinMode(MODE_PIN, INPUT);
        pinMode(RECORD_PIN, INPUT);
        handleModeRecord();

#ifdef DEBUG
        Serial.println("[pin setup] " + String(NUM_POOFERS) + " button -> poofer:");
#endif

        for (short int i = 0; i < NUM_POOFERS; i++) {
                pinMode(poofer[i].pin, OUTPUT);
                pinMode(button[i], INPUT);
                digitalWrite(poofer[i].pin, poofer[i].state);
#ifdef DEBUG
                Serial.println("[pin setup] " + String(button[i]) + " -> " + String(poofer[i].pin));
#endif
        }
}

void serial_setup() {
        Serial.begin(115200);
        Serial.println();
        Serial.println("version: " + String(VERSION));
}

/*
    pattern functions
 */
void pattern1() {
}

/*
    helper functions
 */
void handleModeRecord() {
        static bool old_state = digitalRead(MODE_PIN);
        bool mode_button_state = digitalRead(MODE_PIN);
        static uint32_t last_time = millis();
        uint32_t now = millis();

        if (old_state && !mode_button_state && now - last_time > 50) {
                mode++;
                mode %= 3;
                last_time = now;
#ifdef DEBUG
                Serial.println("[mode] " + String(mode));
#endif
                if (mode == RECORD_MODE) {
                        if (record) record_start = true;
#ifdef DEBUG
                        Serial.println("[record] " + String(record));
#endif
                }
        }

        if (mode_button_state != old_state) {
                old_state = mode_button_state;
        }

        bool record_switch_state = digitalRead(RECORD_PIN);
        if (record_switch_state != record && now - last_time > 50) {
                record = !record;
                last_time = now;
                if (record) record_start = true;
#ifdef DEBUG
                Serial.println("[record] " + String(record));
#endif
        }
}

void handleButtons() {
        static bool last_state[NUM_POOFERS] = { false, false, false, false, false, false };

        for (short int i = 0; i < NUM_POOFERS; i++) {
                bool button_state = digitalRead(button[i]);

                if (button_state != last_state[i]) {
#ifdef DEBUG
                        Serial.println("fire " + String(poofer[i].pin) + ": " + String(button_state));
#endif
                        poofer[i].state = !poofer[i].state;
                        digitalWrite(poofer[i].pin, poofer[i].state);
                        last_button_press = millis();
                        last_state[i] = !last_state[i];
                }
        }
}

void recordPattern() {
        static byte last_state = 0;
        static byte current_state = 0;
        static uint32_t record_start_time = 0;
        static uint32_t last_time = millis();
        uint32_t now = millis();

        if (now - last_time < 100) return;

        current_state = 0;

        for (short int i = 0; i < NUM_POOFERS; i++) {
                current_state |= digitalRead(button[i]) << i;
        }

        if (current_state != last_state) {
                if (record_start) {
                        eeAddr = 0;
                        record_start_time = now;
                        record_start = false;
#ifdef DEBUG
                        Serial.println("[recording] start");
#endif
                }

                Record record = {current_state, now - record_start_time};
                EEPROM.put(eeAddr, record);
#ifdef DEBUG
                Serial.print("[recording] " + String(record.interval) + "ms ");
                Serial.println(current_state, BIN);
                Serial.println("[recording] EEPROM address: " + String(eeAddr));
#endif
                eeAddr += sizeof(record);
                if (eeAddr >= int(EEPROM.length() / sizeof(record)) * sizeof(record)) eeAddr = 0;
                last_state = current_state;
                last_time = now;
        }
}

void playPattern() {
}

void simon() {

}

void pattern(long mill[NUM_POOFERS]) {
}

void idle_pattern() {
}
