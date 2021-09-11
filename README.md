# ThingPulse Icon64/AMo CO2 App

Turn any ESP32-based device into a CO2 monitor with this firmware. Supported CO2 sensors:

- Winsen MH-Z19(C)
- Sensirion SCD4x

The code for both sensors is compiled into the firmware to allow selecting the one you use at runtime.

This is firmware is made for the [ThingPulse Icon64](https://thingpulse.com/product/icon64/) devices of which the [AMo](https://thingpulse.com/product/amo-co2-monitor/) is a specific incarnation.

![](./resources/CO2-monitor-AMo_CO2-ok-green-LEDs.jpg)

## Settings

Change the default settings at the top of `main.cpp` to have them compiled into the binary.

```c++
// ********* user settings *********
int co2WarnLevel = 850;     // unit: ppm
int co2AlertLevel = 1000;   // unit: ppm
...
String co2Sensor = "mhz19"; // [mhz19, scd4x]
```
Furthermore, the firmware supports reading all parameters from `/application.properties` on SPIFFS at startup. This allows to install this CO2 app hassle-free without any development environment through the [ThingPulse App Fairy](https://github.com/ThingPulse/app-fairy) (app store).

## Button

Push the button once to turn off accustic warnings. Push it again - after waiting a couple of seconds - to enable them again. This is not a persistent setting. Hence, turning the device off or otherwise cutting the power will activate the default again.

## Mobile App
Version 1.2 and up publish sensor values over BLE to support the Sensirion MyAmbience app available for iOS and Android. Simply turn the device on, open the MyAmbience app and see your device appear on the dashboard.

![](./resources/Sensirion-MyAmbience.jpg)

<p><a href="https://play.google.com/store/apps/details?id=com.sensirion.myam&amp;hl=en_IE"><img loading="lazy" class="alignleft wp-image-3656 size-full" src="https://thingpulse.com/wp-content/uploads/2021/09/PlayStore.png" alt="" srcset="https://thingpulse.com/wp-content/uploads/2021/09/PlayStore.png 153w, https://thingpulse.com/wp-content/uploads/2021/09/PlayStore-150x46.png 150w" sizes="(max-width: 153px) 100vw, 153px" width="153" height="46"></a><a href="https://apps.apple.com/de/app/sensirion-myambience/id1529131572"><img loading="lazy" class="alignleft wp-image-3657" src="https://thingpulse.com/wp-content/uploads/2021/09/AppStore-300x103.png" alt="" srcset="https://thingpulse.com/wp-content/uploads/2021/09/AppStore-300x103.png 300w, https://thingpulse.com/wp-content/uploads/2021/09/AppStore-768x264.png 768w, https://thingpulse.com/wp-content/uploads/2021/09/AppStore-416x143.png 416w, https://thingpulse.com/wp-content/uploads/2021/09/AppStore.png 976w" sizes="(max-width: 134px) 100vw, 134px" width="134" height="46"></a></p>

## Photos

An Icon64 prototype in classroom use.

![Regular Level](./resources/RegularLevel.jpeg)
![Alert](./resources/FirstAlert.jpeg)
