#pragma once

#include <Arduino.h>
#include <ACAN_ESP32.h>
#include "DCC_Types.h"

class DCC_CANEncoder {
public:
    static void begin();
    static void task(void *pvParameters);
};
