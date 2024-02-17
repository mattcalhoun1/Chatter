#include <Arduino.h>

#ifndef SECRETS_H
#define SECRETS_H

class Secrets {
    public:
        virtual bool init() = 0;
        virtual bool readSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength) = 0;
        virtual bool writeSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength) = 0;
        virtual bool isRunning () = 0;
};

#endif