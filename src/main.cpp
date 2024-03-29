#include <Arduino.h>
#include "SPIFFS.h"
#include <FastLED.h>
#include <EasyButton.h>
#include "AudioGeneratorAAC.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourcePROGMEM.h"
#include "co2_mhz19.h"
#include "co2_scd4x.h"
#include "icons.h"
#include "sounds.h"


// ********* user settings *********
int co2WarnLevel = 850;     // unit: ppm
int co2AlertLevel = 1000;   // unit: ppm
float volume = 0.5;         // {0.0, 4.0}
int brightness = 80;        // {0, 255}
String co2Sensor = "mhz19"; // [mhz19, scd4x]
// ********* END user settings *********


// LED Settings
#define NUM_LEDS      64
#define DATA_PIN      32
#define COLOR_ORDER   GRB

// Audio Settings
#define I2S_DOUT      25
#define I2S_BCLK      26
#define I2S_LRC       22
#define MODE_PIN      33

#define PUSH_BUTTON   39
#define MIN_SOUND_INTERVAL_MILLIS 1000 * 10

// Variables to keep track of when the last action was triggered from loop() in order to achieve
// the intervals defined right below. Variables contain millis() values.
unsigned long acquireDataLoopTimer = 0;
unsigned long ledLoopTimer = 0;
// Intervals at which data acquisition and LED update is activated in loop()
short acquireDataIntervalMs = 5000;
short ledUpdateIntervalMs = 200;

uint16_t numbers[10] = {0x7B6F, 0x1249, 0x73E7, 0x73CF, 0x5BC9, 0x79CF,0x79EF, 0x7249, 0x7BEF, 0x7BCF};

CRGB leds[NUM_LEDS];

uint16_t historicValues[8] = {0};

AudioFileSourcePROGMEM *in;
AudioGeneratorAAC *aac;
AudioOutputI2S *out;
EasyButton button(PUSH_BUTTON);
uint32_t lastSoundPlayed = 0;
bool isMuted = false;
bool showMuteState = false;

int co2Level;   // in ppm, as provided by the sensor
float co2Value; // rounded and reduced value as rendered on the LED matrix

// ********* forward declarations *********
void buttonISR();
void drawCo2Value(CRGB foregroundColor);
void drawDigit(uint8_t x, uint8_t y, uint8_t number, CRGB foregroundColor);
void drawIcon(const uint32_t *icon);
uint8_t getLedIndex(uint8_t x, uint8_t y);
void initCo2Sensor();
void loadPropertiesFromSpiffs();
int measureCo2Level();
void onButtonPressed();
void playBootupSound();
// ********* END forward declarations *********


void setup() {
  Serial.begin(115200);
  delay(4000);
  loadPropertiesFromSpiffs();

  pinMode(MODE_PIN, OUTPUT);
  pinMode(PUSH_BUTTON, INPUT);
  digitalWrite(MODE_PIN, HIGH);

  in = new AudioFileSourcePROGMEM(sound, sizeof(sound));
  aac = new AudioGeneratorAAC();
  out = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT );
  out->SetGain(volume);
  playBootupSound();
  FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  // init button and attach callbacks
  button.begin();
  button.onPressed(onButtonPressed);
  if (button.supportsInterrupt()) {
    button.enableInterrupt(buttonISR);
    log_i("Button will be used through interrupts");
  }

  initBleGadget();
  initCo2Sensor();
}

void loop() {
  if (millis() - acquireDataLoopTimer >= acquireDataIntervalMs) {
    acquireDataLoopTimer = millis();
    int co2Reading = measureCo2Level();
    // values >10k indicate faulty readings (e.g. I2C communication error) -> skip them
    if (co2Reading > 0 && co2Reading < 10000) {
      co2Level = co2Reading;
      co2Value = co2Level / 1000.0;
      if (co2Value < 0.995) {
        // round to 2 fractional digits as we display 2, >=0.995 is rounded up to 1.0
        // Value 0.994000, rounded value 0.990000 -> display ',99'
        // ----
        // Value 0.995000, rounded value 1.000000 -> display '1,0'
        co2Value = round(co2Value * 100.0) / 100.0;
      } else {
        // round to 1 fractional digit as we display only 1
        co2Value = round(co2Value * 10.0) / 10.0;
      }
      log_i("CO2 (ppm): %d, rounded value: %f", co2Level, co2Value);
    }
  }
  if (millis() - ledLoopTimer >= ledUpdateIntervalMs) {
    ledLoopTimer = millis();
    FastLED.clear();
    FastLED.setBrightness(brightness);
    if (showMuteState) {
      if (isMuted) {
        drawIcon(VOL_OFF);
      } else {
        drawIcon(VOL_ON);
      }
      delay(2000);
      showMuteState = false;
    } else {
      CRGB backgroundColor;
      CRGB foregroundColor;
      if (co2Level < co2WarnLevel) {
        backgroundColor = CRGB::Green;
        foregroundColor = CRGB::White;
      } else if (co2Level < co2AlertLevel) {
        backgroundColor = (millis() / 600) % 2 == 0 ? CRGB::Yellow  : CRGB::Black;
        foregroundColor = CRGB::Green;
      } else {
        backgroundColor = (millis() / 200) % 2 == 0 ? CRGB::Red  : CRGB::Black;
        foregroundColor = CRGB::White;
        if (!isMuted && millis() - lastSoundPlayed > MIN_SOUND_INTERVAL_MILLIS) {
          playBootupSound();
          lastSoundPlayed = millis();
        }
      }

      for (int i = 0; i < 64; i++) {
        leds[i] = backgroundColor;
      }

      drawCo2Value(foregroundColor);
    }
    FastLED.show();
  }

  handleBleGadgetEvents();
}

void buttonISR() {
  button.read();
}

void drawDigit(uint8_t x, uint8_t y, uint8_t number, CRGB foregroundColor) {
  number = number % 10;
  uint8_t bitNumber = 0;
  for (uint8_t yk = 0; yk < 5; yk++) {
    for (uint8_t xk = 3; xk > 0; xk--) {
      bool bit = (numbers[number] >> bitNumber) & 1;
      if (bit) {
        leds[getLedIndex(xk + x, yk + y)] = foregroundColor;
      }
      bitNumber++;
    }
  }
}

void drawCo2Value(CRGB foregroundColor) {
  if (co2Value < 1) {
    uint8_t firstDigit = ((int)(co2Value * 10)) % 10;
    uint8_t secondDigit = ((int)(co2Value * 100)) % 10;

    drawDigit(0, 2, firstDigit, foregroundColor);
    drawDigit(4, 2, secondDigit, foregroundColor);
    // draw comma as decimal separator
    leds[getLedIndex(0, 2)] = foregroundColor;
    leds[getLedIndex(0, 1)] = foregroundColor;
  } else {
    uint8_t decimal = ((int)(co2Value * 10)) % 10;
    uint8_t centimal = co2Value;

    drawDigit(-1, 2, centimal, foregroundColor);
    drawDigit(4, 2, decimal, foregroundColor);
    // draw comma as decimal separator
    leds[getLedIndex(4, 2)] = foregroundColor;
    leds[getLedIndex(4, 1)] = foregroundColor;
  }
}

void drawIcon(const uint32_t *icon) {
  for (int i = 0; i < 64; i++) {
    uint32_t pixel = pgm_read_dword(icon + i);
    uint8_t red = (pixel >> 16) & 0xFF;
    uint8_t green = (pixel >> 8) & 0xFF;
    uint8_t blue = pixel & 0xFF;
    leds[getLedIndex(i % 8, i / 8)] = CRGB(red, green, blue);
  }
  delay(1);
  FastLED.show();
}

void initCo2Sensor() {
  if (co2Sensor.equals("mhz19")) {
    initMhz19();
  } else if (co2Sensor.equals("scd4x")) {
    initScd4x();
  }
}

uint8_t getLedIndex(uint8_t x, uint8_t y) {
  if (x % 2 == 1) {
    return 63 - (y + x * 8);
  } else {
    return 63 - (7 - y + x * 8);
  }
}

void loadPropertiesFromSpiffs() {
  if (SPIFFS.begin()) {
    log_i("Attempting to read application.properties file from SPIFFS.");
    File f = SPIFFS.open("/application.properties");
    if (f) {
      log_i("File exists. Reading and assigning properties.");
      while (f.available()) {
        String key = f.readStringUntil('=');
        String value = f.readStringUntil('\n');
        if (key == "co2Sensor") {
          co2Sensor = value;
          log_i("Using 'co2Sensor' from SPIFFS");
        } else if (key == "co2WarnLevel") {
          co2WarnLevel = value.toInt();
          log_i("Using 'co2WarnLevel' from SPIFFS");
        } else if (key == "co2AlertLevel") {
          co2AlertLevel = value.toInt();
          log_i("Using 'co2AlertLevel' from SPIFFS");
        } else if (key == "volume") {
          volume = value.toFloat();
          log_i("Using 'volume' from SPIFFS");
        } else if (key == "brightness") {
          brightness = value.toInt();
          log_i("Using 'brightness' from SPIFFS");
        }
      }
    }
    f.close();
    log_i("Effective properties now as follows:");
    log_i("\tco2Sensor: %s", co2Sensor);
    log_i("\tco2WarnLevel: %d", co2WarnLevel);
    log_i("\tco2AlertLevel: %d", co2AlertLevel);
    log_i("\tvolume: %f", volume);
  } else {
    log_w("SPIFFS mount failed.");
  }
}

int measureCo2Level() {
  if (co2Sensor.equals("mhz19")) {
    return readFromMhz19();
  } else if (co2Sensor.equals("scd4x")) {
    return readFromScd4x();
  } else {
    return 0;
  }
}

void onButtonPressed() {
  isMuted = !isMuted;
  showMuteState = true;
  log_d("Is muted: %d", isMuted);
}

void playBootupSound() {
  log_d("Sound %d", sizeof(sound));
  log_d("Playing sound");
  in->open(sound, sizeof(sound));
  aac->begin(in, out);
  while (aac->isRunning()) {
    aac->loop();
  }
  aac->stop();
  in->close();
  log_d("Finished sound");
}
