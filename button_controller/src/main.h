#define VERSION 2

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

struct Poofer {
        unsigned short int pin;
        bool state;
        bool off;
};

typedef void (*SimplePatternList[])();

struct Record {
        byte poofers;
        uint32_t interval;
};
