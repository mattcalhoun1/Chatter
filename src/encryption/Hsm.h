#include <Arduino.h>

#ifndef HSM_H
#define HSM_H

class Hsm {
    public:
        virtual bool init(const char* clusterId) = 0;
        virtual long getRandomLong() = 0;

        virtual bool loadPublicKey(byte* publicKeyBuffer) = 0;
        virtual bool verifySignature(uint8_t* message, uint8_t* signature, const byte* publicKey) = 0;
        virtual bool sign (uint8_t* message, uint8_t* signatureBuffer) = 0;

        //virtual bool generateNewKeypair () = 0;

        virtual bool generateSymmetricKey (uint8_t* keyBuffer, uint8_t length) = 0;

        virtual void prepareForEncryption (uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize) = 0;
        virtual void prepareForVolatileEncryption (uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize) = 0;

        // encrypt/decrypt, and place the result in the appropriate buffer.
        virtual void encrypt(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize) = 0;
        virtual void decrypt(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) = 0;
        virtual void encryptVolatile(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize) = 0;
        virtual void decryptVolatile(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) = 0;

        virtual bool factoryReset () = 0;
};

#endif