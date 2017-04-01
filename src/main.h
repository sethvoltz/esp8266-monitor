#include <vector>
#include <FS.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// =--------------------------------------------------------------------------------= Constants =--=

// NeoPixel Control
#define SERIAL_BAUDRATE               115200
#define PIXEL_COUNT                   10
#define PIXEL_PIN                     14
#define PIXEL_TYPE                    NEO_GRB + NEO_KHZ400
// NEO_RGB     Pixels are wired for RGB bitstream
// NEO_GRB     Pixels are wired for GRB bitstream, correct for neopixel stick
// NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
// NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip), correct for neopixel stick

// Screen Control
#define SCREEN_COUNT                  20 // Number of screens that can be stored
#define INDICATOR_COLOR               55 // Color as angle [0 <= n < 360]
#define INDICATOR_BRIGHTNESS          128 // Global indicator brightness [0 <= n < 256]
#define MAX_NUM_PARAMS                4

// LED Fading
#define FADE_DURATION_MSEC            250
#define FADE_UPDATE_INTERVAL_MSEC     33 // ~30fps


// =-------------------------------------------------------------------------------= Prototypes =--=

uint32_t Wheel(byte WheelPos, float brightness);
byte scale(byte value, float percent);

void setupDisplay();
void setupConfig();
void saveConfig();

void handleDisplay();
void updateLEDs(unsigned long time_diff);

void handleSerial();
void parseCommand(String line);

void listScreens();
void addScreen(String id, int indicator);
void removeScreen(String id);
void setIndicatorByName(String name);
void setIndicator(int indicator);
