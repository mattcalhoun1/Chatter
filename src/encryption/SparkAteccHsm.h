#include <Arduino.h>

// only for due
#ifdef ARDUINO_SAM_DUE

#include "Hsm.h"
#include "EncryptionGlobals.h"

#include <SparkFun_ATECCX08a_Arduino_Library.h>

#ifndef SPARKATECCHSM_H
#define SPARKATECCHSM_H

class SparkAteccHsm : public Hsm {
    public:
        bool init();
        bool lockDevice (uint8_t defaultPkSlot, uint8_t defaultPkStorage);
        bool generateNewKeypair (uint8_t pkSlot, uint8_t pkStorage);

        long getRandomLong();

        bool loadPublicKey(uint8_t slot, byte* publicKeyBuffer);
        bool verifySignature(uint8_t* message, uint8_t* signature, const byte* publicKey);
        bool sign (uint8_t slot, uint8_t* message, uint8_t* signatureBuffer);

        bool readSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength);
        bool writeSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength);
    
    protected:
        void logConsole(const char* message);
        ATECCX08A atecc;
        uint16_t getEepromAddress (uint8_t slot, uint8_t block, uint8_t offset);
};

#endif

#endif