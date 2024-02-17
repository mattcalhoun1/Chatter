#include <Arduino.h>

#ifndef HSM_H
#define HSM_H

class Hsm {
    public:
        virtual bool init() = 0;
        virtual bool lockDevice (uint8_t defaultPkSlot, uint8_t defaultPkStorage) = 0;
        virtual bool generateNewKeypair (uint8_t pkSlot, uint8_t pkStorage) = 0;

        virtual long getRandomLong() = 0;

        virtual bool loadPublicKey(uint8_t slot, byte* publicKeyBuffer) = 0;
        virtual bool verifySignature(uint8_t* message, uint8_t* signature, const byte* publicKey) = 0;
        virtual bool sign (uint8_t slot, uint8_t* message, uint8_t* signatureBuffer) = 0;

        virtual bool readSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength) = 0;
        virtual bool writeSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength) = 0;
};

#endif