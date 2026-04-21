#pragma once
#include <Arduino.h>

// Entrée DCC logique (sortie XOR SN74LVC1G86)
static const gpio_num_t PIN_DCC_IN = GPIO_NUM_27;

// CAN natif ESP32
static const gpio_num_t PIN_CAN_TX = GPIO_NUM_5;
static const gpio_num_t PIN_CAN_RX = GPIO_NUM_4;

// LED debug
static const gpio_num_t PIN_LED = GPIO_NUM_2;
