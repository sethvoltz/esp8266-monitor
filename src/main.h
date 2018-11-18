#include <vector>
#include <FS.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

// =--------------------------------------------------------------------------------= Constants =--=

// Screen Control
#define SCREEN_COUNT                  20 // Number of screens that can be stored
#define INDICATOR_HUE                 60 // Color as hue angle [0 <= n < 360]
#define INDICATOR_SATURATION          0.95 // [0 <= n <= 1]
#define INDICATOR_INTENSITY           80 // Global indicator brightness [0 <= n < 256]
#define MAX_NUM_PARAMS                4

// NeoPixel Control
#define SERIAL_BAUDRATE               115200
#define PIXELS_PER_SCREEN             3
#define PIXEL_COUNT                   SCREEN_COUNT * PIXELS_PER_SCREEN
#define PIXEL_PIN                     14
#define PIXEL_TYPE                    NEO_GRB + NEO_KHZ400
// NEO_RGB     Pixels are wired for RGB bitstream
// NEO_GRB     Pixels are wired for GRB bitstream, correct for neopixel stick
// NEO_GRBW    Pixels are wired for GRB with W (white) bitstream, correct for all RGBW neopixels
// NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
// NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip), correct for neopixel stick and later

// LED Fading
#define FADE_DURATION_MSEC            250
#define FADE_UPDATE_INTERVAL_MSEC     33 // ~30fps


// =-------------------------------------------------------------------------------= Prototypes =--=

uint32_t Wheel(byte WheelPos, float brightness);
byte scale(byte value, float percent);
float floatmap(float x, float in_min, float in_max, float out_min, float out_max);
uint32_t hsi2rgbw(float H, float S, float I);

void setupDisplay();
void setupConfig();
void saveConfig();
void printConfig();

void handleDisplay();
void updateLEDs(unsigned long time_diff);

void handleSerial();
void parseCommand(String line);

void listScreens();
void showConfig();
void addScreen(String id, int indicator);
void removeScreen(String id);
void setIndicatorByName(String name);
void setIndicator(int indicator);
void setIndicatorColor(float H, float S, int I);
void resetIndicatorColor();
