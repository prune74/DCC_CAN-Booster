#pragma once

#include <Arduino.h>
#include "DCC_Types.h"

/*
   DCC_ISR
   -------
   - Capture les fronts DCC sur IN1/IN2
   - Timestamp via micros()
   - Envoie dans une queue FreeRTOS (déclarée ailleurs)
*/

class DCC_ISR {
public:
    static void begin(uint8_t pinDCC);
    static void IRAM_ATTR handleInterrupt();
};
