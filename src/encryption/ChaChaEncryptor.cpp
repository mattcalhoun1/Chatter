
#include "ChaChaEncryptor.h"

bool ChaChaEncryptor::init () {
    logConsole("ChaCha Encryption");

    bool result = Encryptor::init();
    
    volatileChacha.setKey(volatileEncryptionKey, ENC_SYMMETRIC_KEY_SIZE);

    // preload the symmetric key
    loadEncryptionKey(ENCRYPTION_KEY_SLOT);
    chacha.setKey(encryptionKeyBuffer, ENC_SYMMETRIC_KEY_SIZE);

    // preload the iv
    loadEncryptionKey(ENCRYPTION_IV_SLOT);

    return result;
}

void ChaChaEncryptor::prepareForVolatileEncryption() {
  volatileChacha.clear();
  volatileChacha.setIV(encryptionKeyBuffer, volatileChacha.ivSize());
  memset(this->encryptedBuffer, 0, ENC_ENCRYPTED_BUFFER_SIZE);
  memset(this->unencryptedBuffer, 0, ENC_UNENCRYPTED_BUFFER_SIZE);
}

void ChaChaEncryptor::prepareForEncryption() {
  chacha.clear();
  chacha.setIV(encryptionKeyBuffer, chacha.ivSize());
  memset(this->encryptedBuffer, 0, ENC_ENCRYPTED_BUFFER_SIZE);
  memset(this->unencryptedBuffer, 0, ENC_UNENCRYPTED_BUFFER_SIZE);
}

void ChaChaEncryptor::encrypt(const char* plainText, int len, int slot) {
  // clear any buffers, initialize encryption algo
  prepareForEncryption();

  if (len < ENC_UNENCRYPTED_BUFFER_SIZE) {
    // copy the value to the encrypted buffer
    memcpy(this->unencryptedBuffer, plainText, len); 

    // encrypt the buffer
    chacha.encrypt(this->encryptedBuffer, this->unencryptedBuffer, len);
  } else {
    logConsole("ERROR: cleartext too large for unencrypted buffer");
  }
}

void ChaChaEncryptor::encryptVolatile(const char* plainText, int len) {
  // clear any buffers, initialize encryption algo
  prepareForVolatileEncryption();


  if (len < ENC_UNENCRYPTED_BUFFER_SIZE) {
    // copy the value to the encrypted buffer
    memcpy(this->unencryptedBuffer, plainText, len); 

    // encrypt the buffer
    volatileChacha.encrypt(this->encryptedBuffer, this->unencryptedBuffer, len);
  } else {
    logConsole("ERROR: cleartext too large for unencrypted buffer!");
  }
}

void ChaChaEncryptor::decrypt(uint8_t* encrypted, int len, int slot) {
  // clear any buffers, initialize encryption algo
  prepareForEncryption();

  if (len < ENC_ENCRYPTED_BUFFER_SIZE) {
    // copy encrypted value into encrypted buffer
    memcpy(this->encryptedBuffer, encrypted, len); 

    // decrypt buffer
    chacha.decrypt(this->unencryptedBuffer, this->encryptedBuffer, len);
  } else {
    logConsole("ERROR: encrypted text too large for buffer!");
  }
}

void ChaChaEncryptor::decryptVolatile(uint8_t* encrypted, int len) {
  // clear any buffers, initialize encryption algo
  prepareForVolatileEncryption();

  if (len < ENC_ENCRYPTED_BUFFER_SIZE) {
    // copy encrypted value into encrypted buffer
    memcpy(this->encryptedBuffer, encrypted, len); 

    // decrypt buffer
    volatileChacha.decrypt(this->unencryptedBuffer, this->encryptedBuffer, len);
  } else {
    logConsole("ERROR: encrypted text too large for buffer!");
  }
}