#include <Arduino.h>
#include <EEPROM.h>

#define VERSION 1

#define DEBUG

#define NUM_POOFERS     6
#define DEFAULT_PATTERN 0
#define OFF_PATTERN     1

#define MODE_PIN   A0
#define RECORD_PIN A1

#define DEFAULT_MODE 0
#define IDLE_MODE    0
#define RECORD_MODE  1
#define SIMON_MODE   2

#define IDLE_TIMEOUT 10000

// setup functions
void pin_setup();
void serial_setup();

// pattern functions
void pattern1();
void idle_pattern();

// helper functions
void pattern(long mill[NUM_POOFERS]);
void handleModeRecord();
void handleButtons();
void recordPattern();
void playPattern();
void simon();

struct PooferStruct {
        unsigned short int pin;
        bool state;
};
PooferStruct poofer[NUM_POOFERS] = {
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

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
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
                // pinMode(button[i], INPUT);
                digitalWrite(poofer[i].pin, !poofer[i].state);
                delay(500);
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
#ifdef DEBUG
                Serial.println("[mode] " + String(mode));
                if (mode == RECORD_MODE)
                        Serial.println("[record] " + String(record));
#endif
                last_time = now;
        }

        if (mode_button_state != old_state) {
                old_state = mode_button_state;
        }

        bool record_switch_state = digitalRead(RECORD_PIN);
        if (record_switch_state != record && now - last_time > 50) {
                record = !record;
#ifdef DEBUG
                Serial.println("[record] " + String(record));
                last_time = now;
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

}

void playPattern() {

}

void simon() {

}

void pattern(long mill[NUM_POOFERS]) {
}

void idle_pattern() {
}
