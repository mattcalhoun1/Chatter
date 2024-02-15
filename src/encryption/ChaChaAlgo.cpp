
#include "ChaChaAlgo.h"

ChaChaAlgo::ChaChaAlgo(uint8_t* symmetricKey, uint8_t* volatileKey) {
    chacha.setKey(symmetricKey, ENC_SYMMETRIC_KEY_SIZE);
    volatileChacha.setKey(volatileKey, ENC_SYMMETRIC_KEY_SIZE);
}

bool ChaChaAlgo::init (uint8_t* iv) {
    chacha.setIV(iv, volatileChacha.ivSize());
    volatileChacha.setIV(iv, volatileChacha.ivSize());

    return true;
}

void ChaChaAlgo::prepareForVolatileEncryption(uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize) {
  volatileChacha.clear();
  memset(encryptedBuffer, 0, encryptedBufferSize);
  memset(unencryptedBuffer, 0, unencryptedBufferSize);
}

void ChaChaAlgo::prepareForEncryption(uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize) {
  chacha.clear();
  memset(encryptedBuffer, 0, encryptedBufferSize);
  memset(unencryptedBuffer, 0, unencryptedBufferSize);
}

void ChaChaAlgo::encrypt(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize) {
    // encrypt the buffer
    chacha.encrypt(encryptedBuffer, unencryptedBuffer, len);
}

void ChaChaAlgo::encryptVolatile(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize) {
    volatileChacha.encrypt(encryptedBuffer, unencryptedBuffer, len);
}

void ChaChaAlgo::decrypt(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) {
    chacha.decrypt(unencryptedBuffer, encryptedBuffer, len);
}

void ChaChaAlgo::decryptVolatile(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) {
    // decrypt buffer
    volatileChacha.decrypt(unencryptedBuffer, encryptedBuffer, len);
}