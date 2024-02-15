#include "PassThruAlgo.h"

void PassThruAlgo::prepareForVolatileEncryption(uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize) {
  memset(encryptedBuffer, 0, encryptedBufferSize);
  memset(unencryptedBuffer, 0, unencryptedBufferSize);
}

void PassThruAlgo::prepareForEncryption(uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize) {
  memset(encryptedBuffer, 0, encryptedBufferSize);
  memset(unencryptedBuffer, 0, unencryptedBufferSize);
}

void PassThruAlgo::encrypt(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize) {
    // copy the value to the encrypted buffer, cleartext
    memcpy(encryptedBuffer, unencryptedBuffer, len); 
}

void PassThruAlgo::encryptVolatile(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize) {
    memcpy(encryptedBuffer, unencryptedBuffer, len); 
}

void PassThruAlgo::decrypt(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) {
    memcpy(unencryptedBuffer, unencryptedBuffer, len); 
}

void PassThruAlgo::decryptVolatile(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) {
    memcpy(unencryptedBuffer, unencryptedBuffer, len); 
}

