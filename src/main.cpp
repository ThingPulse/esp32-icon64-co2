#include <Arduino.h>
#include "MHZ19.h"                                        
#include <SoftwareSerial.h>                                // Remove if using HardwareSerial
#include <FastLED.h>

#define RX_PIN 21                                          // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 23                                          // Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600  

// LED Settings
#define NUM_LEDS      64
#define DATA_PIN      32                                    // Device to MH-Z19 Serial baudrate (should not be changed)
#define BRIGHTNESS    20
#define COLOR_ORDER   GRB

MHZ19 myMHZ19;                                             // Constructor for library
SoftwareSerial mySerial(RX_PIN, TX_PIN);                   // (Uno example) create device to MH-Z19 serial

unsigned long getDataTimer = 0;
uint16_t numbers[10] = {0x7B6F, 0x1249, 0x73E7, 0x73CF, 0x5BC9, 0x79CF,0x79EF, 0x7249, 0x7BEF, 0x7BCF};

CRGB leds[NUM_LEDS];

uint16_t historicValues[8] = {0};

CRGB codes[8] = {CRGB::Green, 
                 CRGB::Green,
                 CRGB::YellowGreen, 
                 CRGB::Yellow, 
                 CRGB::Yellow,
                 CRGB::Red,
                 CRGB::Red,
                 CRGB::Red};

uint8_t getLedIndex(uint8_t x, uint8_t y) {
  // 7, 7: 0
  // 7, 6: 1
  // 7, 5: 2
  // 1, 2: 49
  // 1, 1: 48
  // 1, 0: 47
  // 0, 2: 61
  // 0, 1: 62
  // 0, 0: 63
  if (x % 2 == 1) {
    return 63 - (y + x * 8);
  } else {
    return 63 - (7 - y + x * 8);
  }
  /*if (x % 2 == 0) {
    return 63 - (y * 8 + x);
  } else {
    return y * 8 + (7 - x);
  }*/
}

void addValue(uint16_t value) {
  // shift all values left; last value at the end
  for (uint8_t x = 0; x < 7; x++) {
    historicValues[x] = historicValues[x+1];
  }
  historicValues[7] = value;
}

void drawNumber(uint8_t x, uint8_t y, uint8_t number) {
  number = number % 10;
  log_d("NUmber: %d", number);
  uint8_t bitNumber = 0;
  for (uint8_t yk = 0; yk < 5; yk++) {
    for (uint8_t xk = 3; xk > 0; xk--) {
      bool bit = (numbers[number] >> bitNumber) & 1;
      if (bit) {
        leds[getLedIndex(xk + x, yk + y)] = CRGB::White;
      }
      bitNumber++;
    }  
  }
}

void displayValues() {

  CRGB black = CRGB(0x00, 0x00, 0x00);
  for (uint8_t x = 0; x < 8; x++) {
    uint8_t maxY = historicValues[x] / 625;
    //log_d("%d. orig: %d, maxY: %d", x, historicValues[x], maxY);
    for (uint8_t y = 0; y < 8; y++) {
      CRGB color = codes[y];
      if (x == 7 && y == maxY && (millis() / 500) % 2 == 0) {
        color = CRGB::White;
      }
      leds[getLedIndex(x, y)] = y <= maxY ? color : black;
    }
  }

}



void setup()
{
    Serial.begin(115200);                                     // Device to serial monitor feedback

    mySerial.begin(BAUDRATE);                               // (Uno example) device to MH-Z19 serial start   
    myMHZ19.begin(mySerial);                                // *Serial(Stream) refence must be passed to library begin(). 

    myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))

    FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
}

static int counter = 0;

void loop()
{
    if (millis() - getDataTimer >= 2000)
    {
        int CO2; 

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
        addValue(CO2);
        FastLED.clear();
        FastLED.setBrightness(BRIGHTNESS);
        displayValues();
        uint8_t decimal = (CO2 / 100) % 10;
        uint8_t centimal = (CO2 / 1000);
        drawNumber(0, 2, centimal);
        leds[getLedIndex(4, 2)] = CRGB::Grey;
        drawNumber(4, 2, decimal);
        /*leds[1] = CRGB::White;
        leds[62] = CRGB::Green;
        leds[getLedIndex(0,0)] = CRGB::Red;
        leds[getLedIndex(3,0)] = CRGB::Orange;
        leds[getLedIndex(7,7)] = CRGB::Blue;*/
        FastLED.show();
    }

}
