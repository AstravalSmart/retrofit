#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <Arduino.h>
#include "flash_config.h"

const String DID = "d1001";

#define BLEid "IET-233"
// Reset PIN
#define BOOT_BUTTON 0


// Fully usable for digital I/O, PWM, I2C, SPI, etc.
// GPIOs: 22, 25, 26, 27, 32, 33 (input) |||   4, 5, 13, 14, 16, 17, 18, 19, 21, 23 (output)

// Input-Only (No Output Capability)
// GPIOs: 34, 35, 36, 39

#define NODE_COUNT 2 // Number of nodes, can be changed as per requirement
const String nodes[] PROGMEM = {"n0", "n1", "n2", "n3", "n4", "n5", "n6", "n7", "n8", "n9"};
uint8_t outputPins[] PROGMEM = {4, 5, 13, 14, 16, 17, 18, 19, 21, 23};
uint8_t inputPins[] PROGMEM = {22, 25, 26, 27, 32, 33, 34, 35, 36, 37};

void pinModeSetterForDevice();

void pinModeSetterForDevice()
{
    pinMode(BOOT_BUTTON, INPUT_PULLUP); // for reset

    for (int i = 0; i < NODE_COUNT; i++)
    {
        pinMode(outputPins[i], OUTPUT);
        // Read the stored status for the node from preferences & Set the output pin state based on the stored status
        bool status = getLightStatus(nodes[i]);
        digitalWrite(outputPins[i], status ? HIGH : LOW);
        if (inputPins[i] == 34 || inputPins[i] == 35 || inputPins[i] == 36 || inputPins[i] == 39)
        {
            pinMode(inputPins[i], INPUT); // input-only pins
        }
        else
        {
            pinMode(inputPins[i], INPUT_PULLUP);
        }
    }
}

#endif // DEVICE_CONFIG_H

// Fully usable for digital I/O, PWM, I2C, SPI, etc.
// GPIOs: 32, 33, 25, 26, 27 ,22 (input) |||   4, 5, 16, 17, 18, 19, 21, 14, 13, 23 (output)

// Input-Only (No Output Capability)
// GPIOs: 34, 35, 36, 39

// Used for Internal Flash
// GPIOs: 6, 7, 8, 9, 10, 11

// Boot-Affected Pins – Use with Caution
// GPIOs: 0, 2, 12, 15