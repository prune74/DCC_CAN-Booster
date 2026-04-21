#pragma once

#include <Arduino.h>
#include "DCC_Types.h"

class DCC_PacketParser {
public:
    static void begin();
    static void task(void *pvParameters);

private:
    static bool checkCRC(const uint8_t *data, uint8_t size);
};
