#include <Arduino.h>
#include <EEPROM.h>
#include "main.h"

Poofer poofer[NUM_POOFERS] = {
    {23, 39, 25, false, false, false},
    {22, 34, 13, false, false, false},
    {21, 26, 15, false, false, false},
    {19, 33, 4, false, false, false},
    {18, 32, 14, false, false, false},
    {5, 36, 27, false, false, false}
};

uint8_t mode = DEFAULT_MODE;
bool recording = false;
bool record_idle = false;
bool idling = false;
bool off = false;
uint32_t last_button_press = 0;
uint16_t eeAddr = 0;
const Record end_record = {0b10000000, 0};
Button ModeButton, RecordButton;

// List of patterns to cycle through.  Each is defined as a separate function below.
// PatternList gPatterns = { pattern1 };
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current

/*
    SETUP
 */
void setup() {
    serial_setup();
    pin_setup();
    eeprom_setup();
}

/*
    LOOP
 */
void loop() {
    if (off) return;

    handleMode();
    handleRecord();
    switch (mode) {
    case RECORD_MODE:
        if (recording) {
            if (record_idle) {
                recordPattern(Idle);
                // handleButtons();
            }
            else {
                recordPattern(Records);
                // handleButtons();
            }
        }
        else {
            playPattern(Records);
        }
        break;
    case SIMON_MODE:
        simon();
        break;
    default:
        handleButtons();
        if (millis() - last_button_press > IDLE_TIMEOUT) {
            if (!idling) {
                idling = true;
                leds_on_off(0);
#ifdef DEBUG
                Serial.println("[idling] start");
#endif
                eeAddr = 0;
            }
            playPattern(Idle);
        }
        else {
            if (idling) {
                idling = false;
                leds_on_off(1);
#ifdef DEBUG
                Serial.println("[idling] stop");
#endif
            }
        }
    }

}

/*
    SETUP functions
 */
void eeprom_setup() {
    if (!Records.begin(Records.length())) {
        Serial.println("fialed to init Records EEPROM " + String(Records.length()));
        Serial.println("Restarting...");
        delay(1000);
        ESP.restart();
    }
    Serial.println("Records EEPROM initialized: " + String(Records.length()) + " bytes");

    if (!Idle.begin(Idle.length())) {
        Serial.println("fialed to init Idle EEPROM " + String(Idle.length()));
        Serial.println("Restarting...");
        delay(1000);
        ESP.restart();
    }
    Serial.println("Idle EEPROM initialized: " + String(Idle.length()) + " bytes");
}

void pin_setup() {
    ModeButton.pin = MODE_PIN;
    ModeButton.led_pin = 12;
    RecordButton.pin = RECORD_PIN;
    RecordButton.led_pin = 16;
    pinMode(ModeButton.pin, INPUT);
    pinMode(RecordButton.pin, INPUT_PULLUP);
    pinMode(ModeButton.led_pin, OUTPUT);
    pinMode(RecordButton.led_pin, OUTPUT);
    digitalWrite(ModeButton.led_pin, HIGH);
    digitalWrite(RecordButton.led_pin, LOW);
    handleMode();
    handleRecord();

#ifdef DEBUG
    Serial.println("[pin setup] " + String(NUM_POOFERS) + " button -> poofer:");
#endif

    for (short int i = 0; i < NUM_POOFERS; i++) {
        pinMode(poofer[i].pin, OUTPUT);
        pinMode(poofer[i].button_pin, INPUT_PULLUP);
        pinMode(poofer[i].led_pin, OUTPUT);
        digitalWrite(poofer[i].pin, poofer[i].off);
        digitalWrite(poofer[i].led_pin, HIGH);
#ifdef DEBUG
        Serial.println("[pin setup] " + String(poofer[i].button_pin) + " -> " + String(poofer[i].pin));
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

/*
    helper functions
 */
void blink(uint16_t interval, uint8_t times) {
    for (uint8_t n = 1; n <= times; n++) {
        leds_on_off(true);
        delay(interval);
        leds_on_off(false);
        delay(interval);
    }
}

void leds_on_off(bool state) {
    for (uint8_t i = 0; i < NUM_POOFERS; i++) {
        digitalWrite(poofer[i].led_pin, state);
    }
}

uint8_t released(Button &Btn) {
    static uint32_t last_time = millis();
    uint32_t now = millis();

    if (now - last_time < 50) return 0;

    if (digitalRead(Btn.pin) == Btn.pressed) {
        Btn.was_pressed = true;
        last_time = now;
        return 0;
    }

    if (digitalRead(Btn.pin) == !Btn.pressed && Btn.was_pressed) {
        Btn.was_pressed = false;
        last_time = now;
        if (mode == RECORD_MODE && recording && ModeButton.was_pressed) {
            return 2;
        }
        return 1;
    }

    return 0;
}

void handleMode() {
    if (released(ModeButton)) {

        if (mode == RECORD_MODE && recording && record_idle) return;

        mode++;
        mode %= 3;
#ifdef DEBUG
        Serial.println("[mode] " + String(mode));
#endif
        switch (mode) {
        case DEFAULT_MODE:
            recording = false;
            last_button_press = millis();
            leds_on_off(true);
            break;

        case RECORD_MODE:
            eeAddr = 0;
            record_idle = false;
            leds_on_off(false);
#ifdef DEBUG
            Serial.println("[recording] " + String(recording));
#endif
            break;

        case SIMON_MODE:
            recording = false;
            digitalWrite(RecordButton.led_pin, LOW);
            blink(500,2);
            Serial.println("[simon] starting");
            break;
        }
    }
}

void handleRecord() {
    if (mode != RECORD_MODE) return;

    switch (released(RecordButton)) {
    case 1:
        recording = !recording;
        record_idle = false;
        eeAddr = 0;
        digitalWrite(RecordButton.led_pin, recording);
        digitalWrite(ModeButton.led_pin, HIGH);
        leds_on_off(false);
#ifdef DEBUG
        Serial.println("[recording] " + String(recording));
#endif
        // release all poofer relays before recording
        if (recording) {
            for (uint8_t i = 0; i < NUM_POOFERS; i++) {
                poofer[i].state = false;
                digitalWrite(poofer[i].pin, poofer[i].state ^ poofer[i].off);
            }
        }
        break;
    case 2:
        record_idle = !record_idle;
        eeAddr = 0;
        Serial.println("[record idle] " + String(record_idle));
        if (record_idle) {
            digitalWrite(ModeButton.led_pin, LOW);
            blink(100,3);
        }
        else {
            digitalWrite(ModeButton.led_pin, HIGH);
            blink(500,1);
        }
        break;
    }
}

void handleButtons() {
    uint32_t now = millis();

    if (now - last_button_press <= 50) return;

    for (uint8_t i = 0; i < NUM_POOFERS; i++) {
        bool button_state = !digitalRead(poofer[i].button_pin);

        if (button_state != poofer[i].button_state) {
#ifdef DEBUG
            Serial.println("fire " + String(poofer[i].pin) + ": " + String(button_state));
#endif
            poofer[i].state = !poofer[i].state;
            poofer[i].button_state = button_state;
            digitalWrite(poofer[i].pin, poofer[i].state ^ poofer[i].off);
            digitalWrite(poofer[i].led_pin, !button_state);
            last_button_press = millis();
        }
    }
}

void recordPattern(EEPROMClass &Playlist) {
    static byte last_state = 0;
    static byte current_state = 0;
    static uint32_t interval = 0;
    static uint32_t recording_start_time = 0;
    static uint32_t last_time = millis();
    uint32_t now = millis();

    if (now - last_time < 5) return;

    current_state = 0;

    for (uint8_t i = 0; i < NUM_POOFERS; i++) {
        bool button_pressed = !digitalRead(poofer[i].button_pin);
        current_state |= button_pressed << i;
        digitalWrite(poofer[i].led_pin, button_pressed);
    }

    if (current_state != last_state) {
        interval = now - recording_start_time;

        // indicate start of pattern with 0 length interval even when
        // pattern is too long and loops around entire EEPROM
        if (eeAddr == 0) {
            interval = 0;
            recording_start_time = now;
#ifdef DEBUG
            Serial.print("[recording] start");
            if (record_idle) {
                Serial.print(" (IDLE pattern)");
            }
            Serial.println();
#endif
        }

        Record record = {current_state, interval};
        Playlist.put(eeAddr, record);
#ifdef DEBUG
        Serial.printf("[recording] %d ms ", record.interval);
        Serial.println(current_state, BIN);
        Serial.printf("[recording] EEPROM(%d) addr: %d\n", record_idle, eeAddr);
#endif
        eeAddr += sizeof(record);
        if (eeAddr > (int(Playlist.length() / sizeof(record)) * sizeof(record)))
            eeAddr = 0;
        Playlist.put(eeAddr, end_record);
        Playlist.commit();
        last_state = current_state;
        last_time = now;
    }
}

void playPattern(EEPROMClass &Playlist) {
    static Record record;
    static uint32_t last_play = 0;
    uint32_t now = millis();

    Playlist.get(eeAddr, record);

    // check for end record, turn poofers off and start over
    if (record.poofers & end_record.poofers) {
        for (uint8_t i = 0; i < NUM_POOFERS; i++) {
            digitalWrite(poofer[i].pin, poofer[i].off);
            digitalWrite(poofer[i].led_pin, LOW);
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
        for (uint8_t i = 0; i < NUM_POOFERS; i++) {
            bool p = record.poofers & (1 << i);
            if (p != poofer[i].state) {
#ifdef DEBUG
                Serial.print(String(i) + " ");
#endif
                poofer[i].state = !poofer[i].state;
                digitalWrite(poofer[i].pin, poofer[i].state ^ poofer[i].off);
                digitalWrite(poofer[i].led_pin, poofer[i].state);
            }
        }
#ifdef DEBUG
        Serial.println(String(record.interval) + "ms | eeAddr: " + String(eeAddr));
#endif
        eeAddr += sizeof(record);
    }
}

void simon() {

}

void idle_pattern() {
}
