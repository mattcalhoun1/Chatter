#include "FramGlobals.h"
#include <Arduino.h>
#include "Adafruit_FRAM_I2C.h"
#include "FramHardware.h"

#ifndef FRAMADAFRUITI2C_H
#define FRAMADAFRUITI2C_H

class FramAdafruitI2C : public FramHardware {
    public:
        bool init ();

        uint8_t read (uint16_t address);
        bool read (uint16_t address, uint8_t* buffer, uint8_t length);

        bool write (uint16_t address, uint8_t value);
        bool write (uint16_t address, uint8_t* buffer, uint8_t length);    

    protected:
        Adafruit_FRAM_I2C fram;
};

#endif