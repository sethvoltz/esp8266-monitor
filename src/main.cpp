/*
* ==================================================================================================
* The Monitor Monitor - A physical indicator for which display has focus while in fullscreen mode
*
* Author: Seth Voltz
* Created: 14-July-2016
* License: MIT
*
* Original version for Particle microcontroller, ported to ESP8266 1-April-2017.
* ==================================================================================================
*/

#include "main.h"


// =----------------------------------------------------------------------------------= Globals =--=

// Display Pixels
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRBW + NEO_KHZ800);
float indicatorBrightness[SCREEN_COUNT];
int currentIndicator;

// Screens
String screenNames[SCREEN_COUNT];
int screenIndicators[SCREEN_COUNT];

// Color Management
float indicatorH = INDICATOR_HUE;
float targetH = INDICATOR_HUE;
float indicatorS = INDICATOR_SATURATION;
float targetS = INDICATOR_SATURATION;
int indicatorI = INDICATOR_INTENSITY;
int targetI = INDICATOR_INTENSITY;
float colorFadePercent = 0.0;

// Command Buffer
String commandBuffer;


// =----------------------------------------------------------------------------= Configuration =--=

void setupDisplay() {
  // Start NeoPixel Set
  strip.begin();
  setIndicator(-1); // Initialize all pixels to 'off'
}

void setupConfig() {
  // Clean FS, for testing
  // SPIFFS.format();

  // Read configuration from FS json
  Serial.println("Mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("Mounted file system");

    if (SPIFFS.exists("/config.json")) {
      // file exists, reading and loading
      Serial.println("Reading config file");
      File configFile = SPIFFS.open("/config.json", "r");

      if (configFile) {
        Serial.println("Opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        Serial.println();

        if (json.success()) {
          if (json.containsKey("screens")) {
            JsonObject& screens = json["screens"];
            int screen = 0;
            for (JsonObject::iterator it = screens.begin(); it != screens.end(); ++it) {
              screenNames[screen] = it->key;
              screenIndicators[screen] = it->value;
              screen++;
            }
          }

          if (json.containsKey("color")) {
            JsonObject& color = json["color"];
            indicatorH = targetH = color["hue"];
            indicatorS = targetS = color["saturation"];
            indicatorI = targetI = color["intensity"];
          }
        } else {
          Serial.println("Failed to load json config");
        }
      }
    }
  } else {
    Serial.println("Failed to mount FS");
  }
}

void saveConfig() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
  }

  JsonObject& screens = json.createNestedObject("screens");
  for (size_t screen = 0; screen < SCREEN_COUNT; screen++) {
    if (screenNames[screen].length() == 0) {
      break;
    }
    screens[screenNames[screen]] = screenIndicators[screen];
  }

  JsonObject& color = json.createNestedObject("color");
  // Use target so save can be called on set
  color["hue"] = targetH;
  color["saturation"] = targetS;
  color["intensity"] = targetI;

  Serial.println("Saving config...");
  json.printTo(Serial);
  Serial.println();
  json.printTo(configFile);
  configFile.close();
}

void printConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }

  size_t size = configFile.size();
  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);
  DynamicJsonBuffer jsonBuffer;

  JsonObject& json = jsonBuffer.parseObject(buf.get());
  json.prettyPrintTo(Serial);
  Serial.println();

  configFile.close();
}


// =------------------------------------------------------------------------------= LED Display =--=

void handleDisplay() {
  static unsigned long fadeUpdateTimer = millis();

  unsigned long fadeUpdateTimeDiff = millis() - fadeUpdateTimer;
  if (fadeUpdateTimeDiff > FADE_UPDATE_INTERVAL_MSEC) {
    updateLEDs(fadeUpdateTimeDiff);
    fadeUpdateTimer = millis();
  }
}

void updateLEDs(unsigned long time_diff) {
  // current indicator fades to full, all others fade out
  float percent = (float)time_diff / (float)FADE_DURATION_MSEC;
  bool needToWrite = false;
  float currentH, currentS, currentI;

  if (targetH != indicatorH || targetS != indicatorS || targetI != indicatorI) {
    // Step forward
    colorFadePercent += percent;

    if (colorFadePercent >= 1) {
      currentH = indicatorH = targetH;
      currentS = indicatorS = targetS;
      currentI = indicatorI = targetI;
      colorFadePercent = 0;
    } else {
      // Hue
      float diffH = indicatorH - targetH;
      float distanceH = (180.0 - fabs(fmod(fabs(diffH), 360.0) - 180.0)) * ((diffH > 0) ? -1 : 1);
      currentH = indicatorH + distanceH * colorFadePercent;

      // Saturation
      float diffS = targetS - indicatorS;
      currentS = indicatorS + diffS * colorFadePercent;

      // Intensity
      int diffI = targetI - indicatorI;
      currentI = indicatorI + diffI * colorFadePercent;
    }

    needToWrite = true;
  } else {
    currentH = indicatorH;
    currentS = indicatorS;
    currentI = indicatorI;
  }

  for (int indicator = 0; indicator < SCREEN_COUNT; ++indicator) {
    if (indicator == currentIndicator && indicatorBrightness[indicator] < 1) {
      // fade up
      indicatorBrightness[indicator] += percent;
      if (indicatorBrightness[indicator] > 1) indicatorBrightness[indicator] = 1;
      needToWrite = true;
    } else if (indicator != currentIndicator && indicatorBrightness[indicator] > 0) {
      // fade down
      indicatorBrightness[indicator] -= percent;
      if (indicatorBrightness[indicator] < 0) indicatorBrightness[indicator] = 0;
      needToWrite = true;
    }

    // Set all pixels the same
    for(size_t led = 0; led < PIXELS_PER_SCREEN; led++) {
      strip.setPixelColor(
        indicator * PIXELS_PER_SCREEN + led,
        hsi2rgbw(currentH, currentS, indicatorBrightness[indicator])
      );
    }
  }

  if (needToWrite) {
    strip.setBrightness(currentI);
    strip.show();
  }
}


// =-----------------------------------------------------------------------= Command Processing =--=

void handleSerial() {
  while (Serial.available() > 0) {
    char received = Serial.read();

    if (received == '\n') {
      parseCommand(commandBuffer);
      commandBuffer = ""; // Clear received buffer
    } else if (received == '\r') {
      // Ignore.
    } else {
      commandBuffer += received;
    }
  }
}

void parseCommand(String line) {
  String argv[MAX_NUM_PARAMS];
  int argc = 0;

  // Tokenize by spaces
  char* token = strtok(const_cast<char*>(line.c_str()), " ");
  while (token != 0 && argc < MAX_NUM_PARAMS) {
    argv[argc] = String(token);
    token = strtok(0, " ");
    argc++;
  }

  // Check against known commands
  if (argv[0] == "set") {
    if (argc < 2) {
      Serial.println("ERROR: Insufficient parameters");
      return;
    }
    setIndicatorByName(argv[1]);

  } else if (argv[0] == "list") {
    listScreens();

  } else if (argv[0] == "show") {
    showConfig();

  } else if (argv[0] == "add") {
    if (argc < 3) {
      Serial.println("ERROR: Insufficient parameters");
      return;
    }
    addScreen(argv[1], atoi(argv[2].c_str()));

  } else if (argv[0] == "remove") {
    if (argc < 2) {
      Serial.println("ERROR: Insufficient parameters");
      return;
    }
    removeScreen(argv[1]);

  } else if (argv[0] == "color") {
    if (argc < 4) {
      Serial.println("ERROR: Insufficient parameters");
      return;
    }
    setIndicatorColor(atof(argv[1].c_str()), atof(argv[2].c_str()), atoi(argv[3].c_str()));

  } else if (argv[0] == "reset") {
    resetIndicatorColor();

  } else {
    Serial.println("ERROR: Unknown command");
  }
}

void listScreens() {
  Serial.println("OK");

  for (size_t screen = 0; screen < SCREEN_COUNT; screen++) {
    if (screenNames[screen].length() == 0) {
      break;
    }
    Serial.printf("SCREEN: %s (%i)\n", screenNames[screen].c_str(), screenIndicators[screen]);
  }
}

void showConfig() {
  Serial.println("OK");

  printConfig();
}

void addScreen(String name, int indicator) {
  for (size_t screen = 0; screen < SCREEN_COUNT; screen++) {
    if (screenNames[screen] == name || screenNames[screen].length() == 0) {
      screenNames[screen] = name;
      screenIndicators[screen] = indicator;
      saveConfig();
      Serial.println("OK");
      return;
    }
  }

  Serial.println("ERROR: Screen list full");
}

void removeScreen(String name) {
  size_t screen;
  bool found = false;
  for (screen = 0; screen < SCREEN_COUNT; screen++) {
    if (screenNames[screen] == name) {
      screen++;
      found = true;
      break;
    }

    if (screenNames[screen].length() == 0) {
      break;
    }
  }

  if (found) {
    for (size_t bubble = screen; bubble < SCREEN_COUNT; bubble++) {
      screenNames[bubble - 1] = screenNames[bubble];
      screenIndicators[bubble - 1] = screenIndicators[bubble];
      screenNames[bubble] = "";

      if (screenNames[bubble].length() == 0) {
        break;
      }
    }

    // Only save if there was something to remove
    saveConfig();
  }

  Serial.println("OK");
}

void setIndicatorByName(String name) {
  for (size_t screen = 0; screen < SCREEN_COUNT; screen++) {
    if (screenNames[screen].length() == 0) {
      break;
    }

    if (screenNames[screen] == name) {
      setIndicator(screenIndicators[screen]);
      Serial.println("OK");
      return;
    }
  }

  setIndicator(-1);
  Serial.println("ERROR: Unknown screen");
}

void setIndicator(int indicator) {
  currentIndicator = indicator;
}

void setIndicatorColor(float H, float S, int I) {
  targetH = fmod(H, 360); // cycle H around to 0-360 degrees
  targetS = S > 0 ? (S < 1 ? S : 1) : 0; // clamp S and I to interval [0,1]
  targetI = I > 0 ? (I < 255 ? I : 255) : 0;

  saveConfig();

  Serial.println("OK");
}

void resetIndicatorColor() {
  targetH = INDICATOR_HUE;
  targetS = INDICATOR_SATURATION;
  targetI = INDICATOR_INTENSITY;

  saveConfig();

  Serial.println("OK");
}

// =-------------------------------------------------------------------------= Helper Functions =--=

// Same as the Arduino API map() function, except for floats
float floatmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Convert HSI colors to RGBW Neopixel colors
uint32_t hsi2rgbw(float H, float S, float I) {
  int r, g, b, w;
  float cos_h, cos_1047_h;

  H = fmod(H, 360); // cycle H around to 0-360 degrees
  H = 3.14159 * H / (float)180; // Convert to radians.
  S = S > 0 ? (S < 1 ? S : 1) : 0; // clamp S and I to interval [0,1]
  I = I > 0 ? (I < 1 ? I : 1) : 0;

  if(H < 2.09439) {
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667 - H);
    r = S * 255 * I / 3 * (1 + cos_h / cos_1047_h);
    g = S * 255 * I / 3 * (1 + (1 - cos_h / cos_1047_h));
    b = 0;
    w = 255 * (1 - S) * I;
  } else if(H < 4.188787) {
    H = H - 2.09439;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667 - H);
    g = S * 255 * I / 3 * (1 + cos_h / cos_1047_h);
    b = S * 255 * I / 3 * (1 + (1 - cos_h / cos_1047_h));
    r = 0;
    w = 255 * (1 - S) * I;
  } else {
    H = H - 4.188787;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667 - H);
    b = S * 255 * I / 3 * (1 + cos_h / cos_1047_h);
    r = S * 255 * I / 3 * (1 + (1 - cos_h / cos_1047_h));
    g = 0;
    w = 255 * (1 - S) * I;
  }

  return strip.Color(r, g, b, w);
}


// =-------------------------------------------------------------------------= Init and Runtime =--=

void setup() {
  // Init serial port and clean garbage
  Serial.begin(SERIAL_BAUDRATE);

  setupConfig();
  setupDisplay();
}

void loop() {
  handleDisplay();
  handleSerial();
}
