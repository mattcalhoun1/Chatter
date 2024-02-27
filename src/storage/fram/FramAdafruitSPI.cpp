#include "FramAdafruitSPI.h"

bool FramAdafruitSPI::init () {
    if (fram == nullptr) {
        fram = new Adafruit_FRAM_SPI(FRAM_CS);  // use hardware SPI
        return fram->begin(3); // address size in bytes, dependent on which chip
    }

    Serial.println("SPI FRAM already initialized!");
    return false;
}

uint8_t FramAdafruitSPI::read (uint16_t address) {
    return fram->read8(address);
}

bool FramAdafruitSPI::read (uint16_t address, uint8_t* buffer, uint8_t length) {
    return fram->read(address, buffer, length);    
}

bool FramAdafruitSPI::write (uint16_t address, uint8_t value) {
    fram->writeEnable(true);
    bool success = fram->write8(address, value);
    fram->writeEnable(false);

    return success;
}

bool FramAdafruitSPI::write (uint16_t address, uint8_t* buffer, uint8_t length) {
    fram->writeEnable(true);
    bool success = fram->write(address, buffer, length);  
    fram->writeEnable(false);

    return success;
}