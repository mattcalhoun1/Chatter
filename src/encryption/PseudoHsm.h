#include "Hsm.h"
#include "EncryptionGlobals.h"
#include "../storage/DeviceStore.h"
#include "../storage/ClusterStore.h"
#include <Arduino.h>
#include <uECC.h>
#include <SHA256.h>
#include "EncryptionAlgo.h"
#include "ChaChaAlgo.h"

#ifndef PSEUDOHSM_H
#define PSEUDOHSM_H

#define ENC_TEMP_SHARED_SECRET_SIZE 20

class PseudoHsm : public Hsm {
    public:
        PseudoHsm (DeviceStore* _deviceStore, ClusterStore* _clusterStore) { deviceStore = _deviceStore; clusterStore = _clusterStore; }
        bool init(const char* clusterId);

        long getRandomLong();

        bool setClusterId (const char* clusterId);
        const char* getClusterId () {return clusterId;}

        bool loadPublicKey(byte* publicKeyBuffer);
        bool verifySignature(uint8_t* hash, uint8_t* signature, const byte* publicKey);
        bool sign (uint8_t* hash, uint8_t* signatureBuffer);

        void prepareForEncryption (uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize);
        void prepareForVolatileEncryption (uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize);

        void encrypt(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize);
        void decrypt(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize);

        void encryptVolatile(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize);
        void decryptVolatile(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize);

        void encryptForRecipient(const uint8_t* recipientPublicKey, const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize);
        void decryptFromSender(const uint8_t* senderPublicKey, const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize);

        bool generateSymmetricKey (uint8_t* keyBuffer, uint8_t length);

        bool factoryReset ();

        static int rng(uint8_t *dest, unsigned size);
    protected:
        DeviceStore* deviceStore;
        ClusterStore* clusterStore;
        EncryptionAlgo* algo;
        uint8_t symmetricKey[ENC_SYMMETRIC_KEY_SIZE];
        uint8_t iv[ENC_IV_SIZE];

        char clusterId[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE + 1];

        uint8_t signingKey[ENC_PRIV_KEY_SIZE];
        uint8_t publicKey[ENC_PUB_KEY_SIZE];

        uint8_t tempSymmetricKey[ENC_SYMMETRIC_KEY_SIZE*2];
        bool generateTempSymmetricKey (const uint8_t* otherDevicePublicKey, uint8_t keySize);

        const struct uECC_Curve_t * curve = uECC_secp160r1();
        bool generateAndSaveNewKeypair ();
        void generateNextVolatileKey();

        void logConsole(const char* msg);
        uint8_t volatileEncryptionKey[ENC_SYMMETRIC_KEY_BUFFER_SIZE];

        void generateRandomBytes (uint8_t* buffer, int numBytes);

};

#endif