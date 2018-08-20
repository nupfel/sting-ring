#include <Arduino.h>
#include <EEPROM.h>
#include "main.h"

Poofer poofer[NUM_POOFERS] = {
        {2, false, true},
        {3, false, false},
        {4, false, false},
        {5, false, false},
        {6, false, false},
        {7, false, true}
};

const uint8_t button[NUM_POOFERS] = {8, 9, 10, 11, 12, 13};
unsigned short int mode = DEFAULT_MODE;
bool recording = false;
bool off = false;
uint32_t last_button_press = 0;
bool recording_start = false;
uint16_t eeAddr = 0;
const Record end_record = {0b10000000, 0};

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
                if (recording) {
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
                digitalWrite(poofer[i].pin, poofer[i].off);
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
                switch (mode) {
                case RECORD_MODE:
                        if (recording) {
                                recording_start = true;
                        }
                        else {
                                eeAddr = 0;
                        }
#ifdef DEBUG
                        Serial.println("[recording] " + String(recording));
#endif
                        break;
                case SIMON_MODE:
                        break;
                }

        }

        if (mode_button_state != old_state) {
                old_state = mode_button_state;
        }

        bool record_switch_state = digitalRead(RECORD_PIN);
        if (record_switch_state != recording && now - last_time > 50) {
                recording = !recording;
                last_time = now;
                if (recording) {
                        recording_start = true;
                }
                else {
                        eeAddr = 0;
                }
#ifdef DEBUG
                Serial.println("[recording] " + String(recording));
#endif
        }
}

void handleButtons() {
        for (short int i = 0; i < NUM_POOFERS; i++) {
                bool button_state = digitalRead(button[i]);

                if (button_state != poofer[i].state) {
#ifdef DEBUG
                        Serial.println("fire " + String(poofer[i].pin) + ": " + String(button_state));
#endif
                        poofer[i].state = !poofer[i].state;
                        digitalWrite(poofer[i].pin, poofer[i].state ^ poofer[i].off);
                        last_button_press = millis();
                }
        }
}

void recordPattern() {
        static byte last_state = 0;
        static byte current_state = 0;
        static uint32_t recording_start_time = 0;
        static uint32_t last_time = millis();
        uint32_t now = millis();

        if (now - last_time < 5) return;

        current_state = 0;

        for (short int i = 0; i < NUM_POOFERS; i++) {
                current_state |= digitalRead(button[i]) << i;
        }

        if (current_state != last_state) {
                if (recording_start) {
                        eeAddr = 0;
                        recording_start_time = now;
                        recording_start = false;
#ifdef DEBUG
                        Serial.println("[recording] start");
#endif
                }

                Record record = {current_state, now - recording_start_time};
                EEPROM.put(eeAddr, record);
#ifdef DEBUG
                Serial.print("[recording] " + String(record.interval) + "ms ");
                Serial.println(current_state, BIN);
                Serial.println("[recording] EEPROM address: " + String(eeAddr));
#endif
                eeAddr += sizeof(record);
                if (eeAddr >= int(EEPROM.length() / sizeof(record)) * sizeof(record)) eeAddr = 0;
                EEPROM.put(eeAddr, end_record);
                last_state = current_state;
                last_time = now;
        }
}

void playPattern() {
        static Record record;
        static uint32_t last_play = 0;
        uint32_t now = millis();

        EEPROM.get(eeAddr, record);

        // check for end record, turn poofers off and start over
        if (record.poofers & end_record.poofers) {
                for (short int i = 0; i < NUM_POOFERS; i++) {
                        digitalWrite(poofer[i].pin, poofer[i].off);
                }
                eeAddr = 0;
                return;
        }

        // interval == 0 indicates start of a recorded pattern
        if (record.interval == 0 && now >= (PLAY_DELAY + last_play)) {
                if (now - last_play > PLAY_DELAY) {
                        last_play = now;
#ifdef DEBUG
                        Serial.println("[playing] start " + String(last_play) + "ms");
#endif
                }
        }

        if (now >= (PLAY_DELAY + last_play + record.interval)) {
#ifdef DEBUG
                Serial.print("[playing] ");
                Serial.print(record.poofers, BIN);
                Serial.print(": ");
#endif
                for (short int i = 0; i < NUM_POOFERS; i++) {
                        bool p = record.poofers & (1 << i);
                        if (p != poofer[i].state) {
#ifdef DEBUG
                                Serial.print(String(i) + " ");
#endif
                                poofer[i].state = !poofer[i].state;
                                digitalWrite(poofer[i].pin, poofer[i].state ^ poofer[i].off);
                        }
                }
                eeAddr += sizeof(record);
#ifdef DEBUG
                Serial.println("eeAddr: " + String(eeAddr));
#endif
        }
}

void simon() {

}

void pattern(long mill[NUM_POOFERS]) {
}

void idle_pattern() {
}
