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
        bool lockDevice (int defaultPkSlot, int defaultPkStorage);
        bool generateNewKeypair (int pkSlot, int pkStorage);

        long getRandomLong();

        bool loadPublicKey(int slot, byte* publicKeyBuffer);
        bool verifySignature(uint8_t* message, uint8_t* signature, const byte* publicKey);
        bool sign (int slot, uint8_t* message, uint8_t* signatureBuffer);

        bool readSlot(int slot, byte* dataBuffer, int dataLength);
        bool writeSlot(int slot, byte* dataBuffer, int dataLength);
    
    protected:
        void logConsole(const char* message);
        ATECCX08A atecc;
        uint16_t getEepromAddress (int slot, int block, int offset);
};

#endif

#endif