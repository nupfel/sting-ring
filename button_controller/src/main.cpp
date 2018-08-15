// #include <ESP8266WiFi.h>
// #include <ESP8266mDNS.h>
// #include <ESP8266WebServer.h>
#include <Arduino.h>
#include <EEPROM.h>

#define DEBUG

// #define HOSTNAME    "sting.ring"

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
// void eeprom_setup();
void pin_setup();
// void wifi_setup();
// void server_setup();

// pattern functions
void pattern1();
void idle_pattern();

// HTTP endpoint functions
// void HTTP_index();
// void HTTP_on();
// void HTTP_off();
// void HTTP_change_pattern();
// void HTTP_pattern1();

// helper functions
void pattern(long mill[NUM_POOFERS]);
void handleModeRecord();
void handleButtons();
void recordPattern();
void playPattern();
void simon();

// const char *ssid = "Sting Ring";
// const char *password = "BurnMeBaby";
// ESP8266WebServer server(80);
// IPAddress apIP(10, 10, 10, 10);

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

// const char *web_ui = "<html>\n<head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"></head>\n<body style=\"width:100%;height:100%;overflow:hidden;\">\n<svg style=\"width:100%;height:100%;\"\n   viewbox=\"0 0 500 500\" id=\"svg\">\n    <path\n       style=\"fill:#80e5ff;fill-opacity:1;fill-rule:nonzero;stroke:none\"\n       d=\"M 250 0 C 111.92882 3.7895613e-14 0 111.92882 0 250 C -1.249508e-14 341.05067 48.689713 420.72528 121.4375 464.4375 L 154.625 409.40625 C 100.50052 376.95218 64.28125 317.69934 64.28125 250 C 64.28125 147.43284 147.43284 64.28125 250 64.28125 C 352.56716 64.28125 435.71875 147.43284 435.71875 250 C 435.71875 317.53896 399.66155 376.65256 345.75 409.15625 L 378.71875 464.34375 C 451.37991 420.61135 500 340.98541 500 250 C 500 111.92882 388.07118 -1.8947806e-14 250 0 z \" id=\"ring\"/>\n    <rect\n       style=\"fill:#ffffff;fill-opacity:1;fill-rule:nonzero;stroke:none\"\n       id=\"needle\"\n       width=\"16\"\n       height=\"80\"\n       x=\"242\"/>\n    <text\n       xml:space=\"preserve\"\n       style=\"font-size:122.59261322px;font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;text-align:center;line-height:125%;letter-spacing:0px;word-spacing:0px;text-anchor:middle;fill:#000000;fill-opacity:1;stroke:none;font-family:Helvetica;-inkscape-font-specification:Helvetica\"\n       x=\"250.01915\"\n       y=\"845.31812\"\n       id=\"text\"><tspan\n         id=\"label\"\n         x=\"250.01915\"\n         y=\"292.95594\">0</tspan></text>\n    <path\n       style=\"fill:#d5f6ff;fill-opacity:1;fill-rule:nonzero;stroke:none\"\n       id=\"up\"\n       d=\"m 294.75099,133.39225 -90.93056,0 45.46528,-78.748173 z\"\n       transform=\"matrix(0.61903879,0,0,0.61903879,95.682477,91.16682)\"\n       />\n    <path\n       transform=\"matrix(0.61903879,0,0,-0.61903879,95.682477,408.80767)\"\n       d=\"m 294.75099,133.39225 -90.93056,0 45.46528,-78.748173 z\"\n       id=\"dn\"\n       style=\"fill:#d5f6ff;fill-opacity:1;fill-rule:nonzero;stroke:none\" />\n</svg>\n\n<script>\n// Convert touch to mouse event for mobile devices\nfunction touchHandler(event) {\n  var touches = event.changedTouches,\n       first = touches[0], type = \"\";\n  switch(event.type) {\n    case \"touchstart\": type=\"mousedown\"; break;\n    case \"touchmove\":  type=\"mousemove\"; break;        \n    case \"touchend\":   type=\"mouseup\"; break;\n    default: return;\n  }\n  var simulatedEvent = document.createEvent(\"MouseEvent\");\n  simulatedEvent.initMouseEvent(type, true, true, window, 1, \n                            first.screenX, first.screenY, \n                            first.clientX, first.clientY, false, \n                            false, false, false, 0/*left*/, null);\n  first.target.dispatchEvent(simulatedEvent);\n  event.preventDefault();\n}\ndocument.addEventListener(\"touchstart\", touchHandler, true);\ndocument.addEventListener(\"touchmove\", touchHandler, true);\ndocument.addEventListener(\"touchend\", touchHandler, true);\ndocument.addEventListener(\"touchcancel\", touchHandler, true); \n\n// rotate needle to correct position\nvar pos = 0;\nfunction setPos(p) {\n  if (p<0) p=0;\n  if (p>255) p=255;\n  pos = p;\n  document.getElementById(\"label\").textContent = pos;    \n  var a = (pos-127)*1.1;\n  document.getElementById(\"needle\").setAttribute(\"transform\",\"rotate(\"+a+\" 250 250)\");    \n}\nsetPos(pos);\n\n// handle events\nvar dragging = false;\nfunction dragStart() {\n  dragging = true;\n  document.getElementById(\"ring\").style.fill = \"#ff0000\";\n}\ndocument.addEventListener(\"mousemove\", function(e) {\n  if (dragging) {\n    e.preventDefault();\n    var svg = document.getElementById(\"svg\");\n    var ang = Math.atan2(e.clientX-(svg.clientWidth/2),(svg.clientHeight/2)-e.clientY)*180/Math.PI;\n    setPos(Math.round((ang/1.1)+127));\n  }\n});\ndocument.addEventListener(\"mouseup\", function(e) {\n  dragging = false;\n  document.getElementById(\"ring\").style.fill = \"#80e5ff\";\n  document.getElementById(\"up\").style.fill = \"#d5f6ff\";\n  document.getElementById(\"dn\").style.fill = \"#d5f6ff\";\n  var req=new XMLHttpRequest();\n  req.open(\"GET\",\"/pattern/colour?value=\"+pos, true);\n  req.send();\n});\ndocument.getElementById(\"ring\").onmousedown = dragStart;\ndocument.getElementById(\"needle\").onmousedown = dragStart;\ndocument.getElementById(\"up\").onmousedown = function(e) { e.preventDefault(); this.style.fill = \"#ff0000\"; };\ndocument.getElementById(\"dn\").onmousedown = function(e) { e.preventDefault(); this.style.fill = \"#00ff00\"; };\ndocument.getElementById(\"up\").onmouseup = function(e) { setPos(pos+10); };\ndocument.getElementById(\"dn\").onmouseup = function(e) { setPos(pos-10); };\n</script>\n</body>\n</html>";

/*
    SETUP
 */
void setup() {
#ifdef DEBUG
        Serial.begin(115200);
        delay(1000);
        Serial.println();
#endif
        // eeprom_setup();
        pin_setup();
        // wifi_setup();
        // server_setup();
}

/*
    LOOP
 */
void loop() {
        // server.handleClient();

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
// void eeprom_setup() {
//         EEPROM.begin(4096);
// }

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

// void wifi_setup() {
// #ifdef DEBUG
//         Serial.println();
//         Serial.print("[wifi setup] Configuring access point: ");
//         Serial.println(ssid);
// #endif
//
//         WiFi.mode(WIFI_AP);
//         WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
//         WiFi.softAP(ssid, password);
//         WiFi.hostname(HOSTNAME);
//
// #ifdef DEBUG
//         IPAddress myIP = WiFi.softAPIP();
//         Serial.print("[wifi setup] IP address:");
//         Serial.println(myIP);
//
//         WiFi.printDiag(Serial);
// #endif
//
//         if (!MDNS.begin(HOSTNAME)) {
//                 Serial.println("[wifi setup] Error setting up MDNS responder!");
//                 while(1) {
//                         delay(1000);
//                 }
//         }
//
// #ifdef DEBUG
//         Serial.println("[wifi setup] mDNS responder started");
// #endif
// }
//
// void server_setup() {
//         server.on("/", HTTP_index);
//         server.on("/on", HTTP_on);
//         server.on("/off", HTTP_off);
//         server.on("/pattern", HTTP_change_pattern);
//         server.on("/pattern/1", HTTP_pattern1);
//         server.begin();
// #ifdef DEBUG
//         Serial.println("[server setup] HTTP server started");
// #endif
//
//         // Add service to MDNS
//         MDNS.addService("http", "tcp", 80);
// #ifdef DEBUG
//         Serial.println("[server setup] mDNS: HTTP service advertised on port 80");
// #endif
// }

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

/*
    HTTP endpoint functions
 */
// void HTTP_index() {
//         server.send(200, "text/html", web_ui);
// }
//
// void HTTP_on() {
//         off = false;
//         server.send(200, "application/json", "{ \"state\": \"on\" }");
// }
//
// void HTTP_off() {
//         off = true;
//         server.send(200, "application/json", "{ \"state\": \"off\" }");
// }
//
// void HTTP_change_pattern() {
//         gCurrentPatternNumber = server.arg("value").toInt();
//         server.send(200, "application/json", "{ \"pattern\": " + String(gCurrentPatternNumber) + " }");
// }
//
// void HTTP_pattern1() {
//         gCurrentPatternNumber = DEFAULT_PATTERN;
//         server.send(200, "application/json", "{ \"pattern\": \"rainbow\" }");
// }
