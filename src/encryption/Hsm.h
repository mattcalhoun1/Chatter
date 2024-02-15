#include <Arduino.h>

#ifndef HSM_H
#define HSM_H

class Hsm {
    public:
        virtual bool init() = 0;
        virtual bool lockDevice (int defaultPkSlot, int defaultPkStorage) = 0;
        virtual bool generateNewKeypair (int pkSlot, int pkStorage) = 0;

        virtual long getRandomLong() = 0;

        virtual bool loadPublicKey(int slot, byte* publicKeyBuffer) = 0;
        virtual bool verifySignature(uint8_t* message, uint8_t* signature, const byte* publicKey) = 0;
        virtual bool sign (int slot, uint8_t* message, uint8_t* signatureBuffer) = 0;

        virtual bool readSlot(int slot, byte* dataBuffer, int dataLength) = 0;
        virtual bool writeSlot(int slot, byte* dataBuffer, int dataLength) = 0;
};

#endif