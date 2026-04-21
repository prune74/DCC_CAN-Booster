#pragma once

#include <Arduino.h>
#include "DCC_Types.h"

class DCC_Bitstream {
public:
    static void begin();
    static void task(void *pvParameters);
};
