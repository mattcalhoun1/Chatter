#include "EncryptionGlobals.h"
#include <Arduino.h>

#ifndef ENCRYPTIONALGO_H
#define ENCRYPTIONALGO_H

class EncryptionAlgo {
  public:
    virtual bool init (uint8_t* iv) = 0;

    // initialize various buffers, should be called prior to encrypting or decrypting
    // these methods will wipe both buffers
    virtual void prepareForEncryption (uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize) = 0;
    virtual void prepareForVolatileEncryption (uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize) = 0;

    // encrypt/decrypt, and place the result in the appropriate buffer.
    virtual void encrypt(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize) = 0;
    virtual void decrypt(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) = 0;
    virtual void encryptVolatile(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize) = 0;
    virtual void decryptVolatile(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) = 0;
};

#endif