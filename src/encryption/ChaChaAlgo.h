#include <Arduino.h>
#include "EncryptionAlgo.h"
#include "EncryptionGlobals.h"
#include <ChaCha.h>

#ifndef CHACHAALGO_H
#define CHACHAALGO_H

class ChaChaAlgo : public EncryptionAlgo {
  public:
    ChaChaAlgo(uint8_t* symmetricKey, uint8_t* volatileKey);
    bool init (uint8_t* iv);

    void encrypt(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize);
    void decrypt(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize);
    void encryptVolatile(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize);
    void decryptVolatile(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize);

    void prepareForEncryption (uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize);
    void prepareForVolatileEncryption (uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize);

  protected:
    void logConsole(const char* msg);
    uint8_t iv[ENC_IV_SIZE];

    ChaCha chacha;
    ChaCha volatileChacha;
};

#endif