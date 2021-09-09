// GadgetBle snippets extracted from https://github.com/Sensirion/arduino-ble-gadget/blob/master/examples/Example8_SCD4x_BLE_Gadget_with_RHT/Example8_SCD4x_BLE_Gadget_with_RHT.ino

#ifndef BLEGADGET_H
#define BLEGADGET_H

#include "Sensirion_GadgetBle_Lib.h"


GadgetBle gadgetBle = GadgetBle(GadgetBle::DataType::T_RH_CO2);

void initBleGadget() {
    gadgetBle.begin();
    log_i("Sensirion GadgetBle Lib initialized with deviceId = %s", gadgetBle.getDeviceIdString().c_str());
}

void writeToBleGadget(uint16_t co2, float temperature, float humidity) {
    gadgetBle.writeCO2(co2);
    gadgetBle.writeTemperature(temperature);
    if (humidity >= 0) {
        gadgetBle.writeHumidity(humidity);
    }

    gadgetBle.commit();
}

void handleBleGadgetEvents() {
    gadgetBle.handleEvents();
    // The lib documentation at https://github.com/Sensirion/arduino-ble-gadget says
    // "Keep the loop delay at 3ms...Increasing this delay will slow down the download process."
    // "To allow history data downloads, you need to have the handleEvents function within the loop."
    delay(3);
}

#endif /*BLEGADGET_H*/

