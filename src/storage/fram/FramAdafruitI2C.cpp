#include "FramAdafruitI2C.h"

bool FramAdafruitI2C::init () {
    return fram.begin();
}

uint8_t FramAdafruitI2C::read (uint16_t address) {
    return fram.read(address);
}

bool FramAdafruitI2C::read (uint16_t address, uint8_t* buffer, uint8_t length) {
    return fram.read(address, buffer, length);    
}

bool FramAdafruitI2C::write (uint16_t address, uint8_t value) {
    return fram.write(address, value);   
}

bool FramAdafruitI2C::write (uint16_t address, uint8_t* buffer, uint8_t length) {
    return fram.write(address, buffer, length);  
}