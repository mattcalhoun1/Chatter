#include "FramHardware.h"
#include "FramGlobals.h"
#include "Adafruit_FRAM_SPI.h"

#ifndef FRAMADAFRUITSPI_H
#define FRAMADAFRUITSPI_H

class FramAdafruitSPI : public FramHardware {
    public:
        bool init ();

        uint8_t read (uint16_t address);
        bool read (uint16_t address, uint8_t* buffer, uint8_t length);

        bool write (uint16_t address, uint8_t value);
        bool write (uint16_t address, uint8_t* buffer, uint8_t length);    
    protected:
        Adafruit_FRAM_SPI* fram = nullptr;        
};

#endif