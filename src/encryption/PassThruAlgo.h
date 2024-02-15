#include "EncryptionAlgo.h"
#include <Arduino.h>

#ifndef PASSTHRUALGO_H
#define PASSTHRUALGO_H

class PassThruAlgo : public EncryptionAlgo {
    bool init (uint8_t* iv) {return true;}

    void encrypt(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize);
    void decrypt(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize);
    void encryptVolatile(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize);
    void decryptVolatile(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize);

    void prepareForEncryption (uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize);
    void prepareForVolatileEncryption (uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize);

};
#endif