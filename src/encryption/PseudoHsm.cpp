#include "PseudoHsm.h"

bool PseudoHsm::init(const char* _clusterId) {
    uECC_set_rng(&RNG);

    // if there is no private key yet, generate a new keypair
    bool success = false;
    success = deviceStore->loadSigningKey(signingKey);
    if (!success) {
        success = generateAndSaveNewKeypair();
    }

    if (success) {
        // attempt to load cluster symmetric key
        memcpy(this->clusterId, _clusterId, STORAGE_LOCAL_NET_ID_SIZE + STORAGE_GLOBAL_NET_ID_SIZE);
        if(deviceStore->loadSymmetricKey (clusterId, symmetricKey)) {
            logConsole("Cluster key loaded");
            if (deviceStore->loadIv(clusterId, iv)) {
                logConsole("IV prepared");
            }
            else {
                logConsole("IV failed");
                return false;
            }
        }
        else {
            logConsole("Symmetric key load failed");
            return false;
        }

        generateNextVolatileKey();

        algo = new ChaChaAlgo(this->symmetricKey, this->volatileEncryptionKey);
        algo->init(this->iv);
    }
    else {
        logConsole("Error loading symmetric key");
        return true;
    }

    logConsole("Signing key loaded.");
    return uECC_compute_public_key(signingKey, publicKey, curve) != 0;
}

void PseudoHsm::prepareForEncryption (uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize) {
    algo->prepareForEncryption(unencryptedBuffer, unencryptedBufferSize, encryptedBuffer, encryptedBufferSize);
}

void PseudoHsm::prepareForVolatileEncryption (uint8_t* unencryptedBuffer, int unencryptedBufferSize, uint8_t* encryptedBuffer, int encryptedBufferSize) {
    algo->prepareForVolatileEncryption (unencryptedBuffer, unencryptedBufferSize, encryptedBuffer, encryptedBufferSize);
}

// encrypt/decrypt, and place the result in the appropriate buffer.
void PseudoHsm::encrypt(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize) {
    algo->encrypt(unencryptedBuffer, len, encryptedBuffer, encryptedBufferSize);
}

void PseudoHsm::decrypt(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) {
    algo->decrypt(encryptedBuffer, len, unencryptedBuffer, unencryptedBufferSize);
}

void PseudoHsm::encryptVolatile(const uint8_t* unencryptedBuffer, int len, uint8_t* encryptedBuffer, int encryptedBufferSize) {
    algo->encryptVolatile(unencryptedBuffer, len, encryptedBuffer, encryptedBufferSize);
}

void PseudoHsm::decryptVolatile(const uint8_t* encryptedBuffer, int len, uint8_t* unencryptedBuffer, int unencryptedBufferSize) {
    algo->decryptVolatile(encryptedBuffer, len, unencryptedBuffer, unencryptedBufferSize);
}


static int RNG(uint8_t *dest, unsigned size) {
  // Use the least-significant bits from the ADC for an unconnected pin (or connected to a source of 
  // random noise). This can take a long time to generate random data if the result of analogRead(0) 
  // doesn't change very frequently.
  while (size) {
    uint8_t val = 0;
    for (unsigned i = 0; i < 8; ++i) {
      int init = analogRead(0);
      int count = 0;
      while (analogRead(0) == init) {
        ++count;
      }
      
      if (count == 0) {
         val = (val << 1) | (init & 0x01);
      } else {
         val = (val << 1) | (count & 0x01);
      }
    }
    *dest = val;
    ++dest;
    --size;
  }
  // NOTE: it would be a good idea to hash the resulting random data using SHA-256 or similar.
  return 1;
}

void PseudoHsm::generateNextVolatileKey () {
    uint8_t generatedBytes = 0;
    while (generatedBytes < ENC_VOLATILE_KEY_SIZE) {
        volatileEncryptionKey[generatedBytes++] = (uint8_t)getRandomLong();
    }
    volatileEncryptionKey[generatedBytes] = 0;
}

bool PseudoHsm::generateAndSaveNewKeypair() {
    logConsole("Generating new keypair");

    if (uECC_make_key(publicKey, signingKey, curve)) {
        logConsole("Keypair generated. Saving...");
        if (deviceStore->setSigningKey(signingKey)) {
            logConsole("Saved signing key.");
            return true;
        }
        else {
            logConsole("Error saving key");
            return false;
        }
    }

    logConsole("Keypair generate failed");
    return false;
}

long PseudoHsm::getRandomLong() {
    return random();
}

bool PseudoHsm::loadPublicKey(byte* publicKeyBuffer) {
    memcpy(publicKeyBuffer, publicKey, ENC_PUB_KEY_SIZE);
}

bool PseudoHsm::verifySignature(uint8_t* hash, uint8_t* signature, const byte* publicKey) {
    return uECC_verify(publicKey, hash, ENC_HASH_SIZE, signature, curve) == 1;
}

bool PseudoHsm::sign (uint8_t* hash, uint8_t* signatureBuffer) {
    return uECC_sign(signingKey, hash, ENC_HASH_SIZE, signatureBuffer, curve) == 1;
}

void PseudoHsm::logConsole(const char* msg) {
    if (ENC_LOG_ENABLED) {
        Serial.println(msg);
    }
}