#include <Arduino.h>
#ifndef FRAMHARDWARE_H
#define FRAMHARDWARE_H

class FramHardware {
    public:
        virtual bool init () = 0;

        virtual uint8_t read (uint16_t address) = 0;
        virtual bool read (uint16_t address, uint8_t* buffer, uint8_t length) = 0;

        virtual bool write (uint16_t address, uint8_t value) = 0;
        virtual bool write (uint16_t address, uint8_t* buffer, uint8_t length) = 0;
};


#endif