
#include "ChaChaAlgo.h"

ChaChaAlgo::ChaChaAlgo(uint8_t* symmetricKey, uint8_t* volatileKey) {
    chacha.setKey(symmetricKey, ENC_SYMMETRIC_KEY_SIZE);
    volatileChacha.setKey(volatileKey, ENC_SYMMETRIC_KEY_SIZE);
}

bool ChaChaAlgo::init (uint8_t* iv) {
    memcpy(this->iv, iv, ENC_IV_SIZE);
    logConsole("ChaCha Algo Configured");
    return true;
}

void ChaChaAlgo::prepareForVolatileEncryption(uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize) {
    volatileChacha.clear();
    volatileChacha.setIV(iv, ENC_IV_SIZE);
    memset(encryptedBuffer, 0, encryptedBufferSize);
    memset(unencryptedBuffer, 0, unencryptedBufferSize);
}

void ChaChaAlgo::prepareForEncryption(uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize) {
    chacha.clear();
    chacha.setIV(iv, ENC_IV_SIZE);
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

void ChaChaAlgo::encryptUsingSecret(const uint8_t* sharedSecret16, const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize) {
    tempChacha.clear();
    tempChacha.setKey(sharedSecret16, ENC_SYMMETRIC_KEY_SIZE);
    tempChacha.setIV(iv, ENC_IV_SIZE);
    tempChacha.encrypt(encryptedBuffer, unencryptedBuffer, len);
    tempChacha.clear();
}

void ChaChaAlgo::decryptUsingSecret(const uint8_t* sharedSecret16, const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) {
    tempChacha.clear();
    tempChacha.setKey(sharedSecret16, ENC_SYMMETRIC_KEY_SIZE);
    tempChacha.setIV(iv, ENC_IV_SIZE);
    tempChacha.decrypt(unencryptedBuffer, encryptedBuffer, len);
    tempChacha.clear();
}

void ChaChaAlgo::decrypt(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) {
    chacha.decrypt(unencryptedBuffer, encryptedBuffer, len);
}

void ChaChaAlgo::decryptVolatile(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) {
    // decrypt buffer
    volatileChacha.decrypt(unencryptedBuffer, encryptedBuffer, len);
}

void ChaChaAlgo::logConsole(const char* msg) {
    if (ENC_LOG_ENABLED) {
        Serial.println(msg);
    }
}

bool ChaChaAlgo::generateSymmetricKey (uint8_t* keyBuffer, uint8_t length) {
    randomSeed(millis());
    for (uint8_t i = 0; i < length; i++) {
        keyBuffer[i] = random(0, 256);

        // reseed just to be a little more random
        randomSeed(millis() - keyBuffer[i]);
    }

    return true;
}