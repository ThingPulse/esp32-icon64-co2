# ThingPulse Icon64 CO2 Device

ESP32 based device to measure CO2 values with a MH-Z19(B/C) sensor. The device has to levels of alert.
When the first level is reached, the device will start blinking and the colors are switched to yellow.

When the second threshold is reached the colors switch to red and an accoustic signal will be emmitted.

The button on the Icon64 can be used to turn of the accoustic signal.

This is firmware is made for the [ThingPulse Icon64](https://thingpulse.com/product/icon64/) devices.

## Settings

The following values in main.cpp can be adjusted to change the threshold values:

```
#define CO2_WARN_LEVEL 700
#define CO2_ALERT_LEVEL 850
```

## Button

The button can be used to turn off accustic warnings.

## Photos

![Regular Level](/resources/RegularLevel.jpeg)
![Alert](/resources/FirstAlert.jpeg)