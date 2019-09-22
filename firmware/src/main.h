#define VERSION 3

#define DEBUG

#define NUM_POOFERS     6
#define DEFAULT_PATTERN 0
#define OFF_PATTERN     1

#define MODE_PIN   35
#define RECORD_PIN 17

#define DEFAULT_MODE 0
#define IDLE_MODE    0
#define RECORD_MODE  1
#define SIMON_MODE   2

#define IDLE_SIZE    1024
#define IDLE_TIMEOUT 10000
#define PLAY_DELAY   1500

struct Button {
    uint8_t pin;
    uint8_t led_pin;
    bool    pressed = false;
    bool    was_pressed = false;
};

struct Poofer {
    uint8_t pin;
    uint8_t button_pin;
    uint8_t led_pin;
    bool    state;
    bool    off;
    bool    button_state;
};

// typedef void (*PatternList[])();

struct Record {
    byte     poofers;
    uint32_t interval;
};

EEPROMClass Idle("eeprom0", IDLE_SIZE);
EEPROMClass Records("eeprom1", 4096 - IDLE_SIZE);

// setup functions
void pin_setup();
void serial_setup();
void eeprom_setup();

// pattern functions
void idle_pattern();

// helper functions
void blink(uint16_t, uint8_t);
void leds_on_off(bool);
uint8_t released(Button&);
void handleMode();
void handleRecord();
void handleButtons();
void recordPattern(EEPROMClass&);
void playPattern(EEPROMClass&);
void simon();
