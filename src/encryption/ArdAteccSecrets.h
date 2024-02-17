// DUE does not like the arduino atecc library
#ifndef ARDUINO_SAM_DUE

#include <Arduino.h>
#include "Secrets.h"
#include <ArduinoECCX08.h>

#ifndef ARDATECCSECRETS_H
#define ARDATECCSECRETS_H

class ArdAteccSecrets : public Secrets {
    public:
        bool init();
        bool readSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength);
        bool writeSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength);
        bool isRunning() {return running;}
    protected:
        bool running = false;

};

#endif

#endif