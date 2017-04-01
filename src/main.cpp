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
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);
float indicatorBrightness[PIXEL_COUNT];
int currentIndicator;

// Screens
String screenNames[SCREEN_COUNT];
int screenIndicators[SCREEN_COUNT];

// Command Buffer
String commandBuffer;


// =-------------------------------------------------------------------------= Helper Functions =--=

// Input a value 0 to 255 to get a color value.
// Brightness percent between 0 and 1
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos, float brightness) {
  if (brightness == 0) {
    return strip.Color(0, 0, 0);
  }

  if (WheelPos < 85) {
    return strip.Color(
      scale(WheelPos * 3, brightness),
      scale(255 - WheelPos * 3, brightness),
      scale(0, brightness)
    );
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(
      scale(255 - WheelPos * 3, brightness),
      scale(0, brightness),
      scale(WheelPos * 3, brightness)
    );
  } else {
    WheelPos -= 170;
    return strip.Color(
      scale(0, brightness),
      scale(WheelPos * 3, brightness),
      scale(255 - WheelPos * 3, brightness)
    );
  }
}

byte scale(byte value, float percent) {
  return map(value, 0, 255, 0, (int)(percent * 255));
}


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
          int screen = 0;
          for (JsonObject::iterator it = json.begin(); it != json.end(); ++it) {
            screenNames[screen] = it->key;
            screenIndicators[screen] = it->value;
            screen++;
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

  for (size_t screen = 0; screen < SCREEN_COUNT; screen++) {
    if (screenNames[screen].length() == 0) {
      break;
    }
    json[screenNames[screen]] = screenIndicators[screen];
  }

  Serial.println("Saving config...");
  json.printTo(Serial);
  Serial.println();
  json.printTo(configFile);
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

  for (int i = 0; i < PIXEL_COUNT; ++i) {
    if (i == currentIndicator && indicatorBrightness[i] < 1) {
      // fade up
      indicatorBrightness[i] += percent;
      if (indicatorBrightness[i] > 1) indicatorBrightness[i] = 1;
      needToWrite = true;
    } else if (i != currentIndicator && indicatorBrightness[i] > 0) {
      // fade down
      indicatorBrightness[i] -= percent;
      if (indicatorBrightness[i] < 0) indicatorBrightness[i] = 0;
      needToWrite = true;
    }
    strip.setPixelColor(i, Wheel(INDICATOR_COLOR, indicatorBrightness[i]));
  }

  if (needToWrite) {
    strip.setBrightness(INDICATOR_BRIGHTNESS);
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
