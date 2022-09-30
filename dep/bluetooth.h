#pragma once

#include "boardLed.h"
#include "pinouts.h"

#ifdef BT_ON
#include "../lib/arduino-esp32/libraries/BluetoothSerial/src/BluetoothSerial.h"
BluetoothSerial serialBT;
#endif

void pinoutInitAndOpenBTSerialBluetooth() {
#ifdef BT_ON
  serialBT.begin("ESP32Test");
  Serial.println("Bluetooth configured, now you can pair it!");
#endif
}

void btSend(int message) {
#ifdef BT_ON
  serialBT.write(message);
#endif
}

int btRecieve() {
#ifdef BT_ON
  if (serialBT.available()) {
    int message = serialBT.read();
    return message;
  }
#endif
  return -1;
}