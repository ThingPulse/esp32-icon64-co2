#include <Arduino.h>
#include "MHZ19.h"                                        
#include <SoftwareSerial.h>                                // Remove if using HardwareSerial
#include <FastLED.h>
#include <EasyButton.h>
#include "AudioGeneratorAAC.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourcePROGMEM.h"
#include "sounds.h"
#include "icons.h"

#define RX_PIN 21                                          // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 23                                          // Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600  

#define CO2_WARN_LEVEL 700
#define CO2_ALERT_LEVEL 850

// LED Settings
#define NUM_LEDS      64
#define DATA_PIN      32                                    // Device to MH-Z19 Serial baudrate (should not be changed)
#define BRIGHTNESS    20
#define COLOR_ORDER   GRB

// Audio Settings
#define I2S_DOUT      25
#define I2S_BCLK      26
#define I2S_LRC       22
#define MODE_PIN      33

#define PUSH_BUTTON   39
#define MIN_SOUND_INTERVAL_MILLIS 1000 * 10

MHZ19 myMHZ19;                                             // Constructor for library
SoftwareSerial mySerial(RX_PIN, TX_PIN);                   // (Uno example) create device to MH-Z19 serial

unsigned long getDataTimer = 0;
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

int CO2; 

void onButtonPressed() {
  isMuted = !isMuted;
  showMuteState = true;
  log_d("Is muted: %d", isMuted);
}

void buttonISR() {
  button.read();
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

uint8_t getLedIndex(uint8_t x, uint8_t y) {
  if (x % 2 == 1) {
    return 63 - (y + x * 8);
  } else {
    return 63 - (7 - y + x * 8);
  }
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

void drawDouble(double value, CRGB foregroundColor) {
  if (value < 1) {
    leds[getLedIndex(0, 2)] = foregroundColor;
    leds[getLedIndex(0, 1)] = foregroundColor;
    uint8_t firstDigit = ((int)(value * 10)) % 10;
    uint8_t secondDigit = ((int)(value * 100)) % 10;
    drawDigit(0, 2, firstDigit, foregroundColor);
    drawDigit(4, 2, secondDigit, foregroundColor);
  } else {
    uint8_t decimal = ((int)(value * 10)) % 10;
    uint8_t centimal = value;
      
    drawDigit(-1, 2, centimal, foregroundColor);
    drawDigit(4, 2, decimal, foregroundColor);
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

void setup() {
    Serial.begin(115200);                                     // Device to serial monitor feedback
    pinMode(MODE_PIN, OUTPUT);
    pinMode(PUSH_BUTTON, INPUT);
    digitalWrite(MODE_PIN, HIGH);

    mySerial.begin(BAUDRATE);                               // (Uno example) device to MH-Z19 serial start   
    myMHZ19.begin(mySerial);                                // *Serial(Stream) refence must be passed to library begin(). 

    myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))

    in = new AudioFileSourcePROGMEM(sound, sizeof(sound));
    aac = new AudioGeneratorAAC();
    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT );
    out->SetGain(0.5);
    playBootupSound();
    FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);

    // init button and attach callbacks
    button.begin();
    button.onPressed(onButtonPressed);
    if (button.supportsInterrupt()) {
      button.enableInterrupt(buttonISR);
      Serial.println("Button will be used through interrupts");
    }
}


void loop()
{
    if (millis() - getDataTimer >= 2000) {

        /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even 
        if below background CO2 levels or above range (useful to validate sensor). You can use the 
        usual documented command with getCO2(false) */

        CO2 = myMHZ19.getCO2();                             // Request CO2 (as ppm)
        
        Serial.print("CO2 (ppm): ");                      
        Serial.println(CO2);                                

        int8_t Temp;
        Temp = myMHZ19.getTemperature();                     // Request Temperature (as Celsius)
        Serial.print("Temperature (C): ");                  
        Serial.println(Temp);                               

        getDataTimer = millis();
    }
    FastLED.clear();
    FastLED.setBrightness(BRIGHTNESS);
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
      if (CO2 < CO2_WARN_LEVEL) {
        backgroundColor = CRGB::Green;
        foregroundColor = CRGB::White;
      } else if (CO2 < CO2_ALERT_LEVEL) {
        backgroundColor = (millis() / 500) % 2 == 0 ? CRGB::Yellow  : CRGB::Black;
        foregroundColor = CRGB::Green;

      } else {
        backgroundColor = (millis() / 250) % 2 == 0 ? CRGB::Red  : CRGB::Black;
        foregroundColor = CRGB::White;
        if (!isMuted && millis() - lastSoundPlayed > MIN_SOUND_INTERVAL_MILLIS) {
          playBootupSound();
          lastSoundPlayed = millis();
        }

      }


      for (int i = 0; i < 64; i++) {
        leds[i] = backgroundColor;
      }

      drawDouble(CO2 / 1000.0, foregroundColor);
    }
    FastLED.show();
    

}
