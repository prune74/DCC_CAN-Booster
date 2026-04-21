#include "DCC_ISR.h"
#include "freertos/queue.h"

// Queue fournie par Task.cpp
extern QueueHandle_t dccEdgesQueue;

static uint8_t _pinDCC = 0;

void DCC_ISR::begin(uint8_t pinDCC)
{
    _pinDCC = pinDCC;
    pinMode(_pinDCC, INPUT);

    // Attache l'interruption sur les deux fronts
    attachInterrupt(
        digitalPinToInterrupt(_pinDCC),
        DCC_ISR::handleInterrupt,
        CHANGE);
}

void IRAM_ATTR DCC_ISR::handleInterrupt()
{
    DCCEdge edge;
    edge.timestamp = micros();
    edge.level = digitalRead(_pinDCC);

    // Envoi non bloquant dans la queue
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(dccEdgesQueue, &edge, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}
