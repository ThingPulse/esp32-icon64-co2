#include "MHZ19.h"
#include <SoftwareSerial.h>                               // Remove if using HardwareSerial

#include "bleGadget.h"

#define RX_PIN 21                                         // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 23                                         // Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600                                     // Device to MH-Z19 Serial baudrate (should not be changed)

MHZ19 myMHZ19;                                            // Constructor for library
SoftwareSerial mySerial(RX_PIN, TX_PIN);                  // (Uno example) create device to MH-Z19 serial

void initMhz19() {
  mySerial.begin(BAUDRATE);                               // (Uno example) device to MH-Z19 serial start
  myMHZ19.begin(mySerial);                                // *Serial(Stream) refence must be passed to library begin().

  myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))
}

int readFromMhz19() {
  /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even
    if below background CO2 levels or above range (useful to validate sensor). You can use the
    usual documented command with getCO2(false) */
  uint16_t co2 = myMHZ19.getCO2();                        // Request CO2 (as ppm)
  float temperature = myMHZ19.getTemperature();
  float humidity = -1;
  log_i("CO2: %d, Temperature: %f", co2, temperature);
  writeToBleGadget(co2, temperature, humidity);
  return co2;
}
