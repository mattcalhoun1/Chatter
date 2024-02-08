#include "PassThruEncryptor.h"

bool PassThruEncryptor::init() {
  return Encryptor::init();
}

void PassThruEncryptor::prepareForVolatileEncryption() {
  memset(this->encryptedBuffer, 0, ENC_ENCRYPTED_BUFFER_SIZE);
  memset(this->unencryptedBuffer, 0, ENC_UNENCRYPTED_BUFFER_SIZE);
}

void PassThruEncryptor::prepareForEncryption() {
  memset(this->encryptedBuffer, 0, ENC_ENCRYPTED_BUFFER_SIZE);
  memset(this->unencryptedBuffer, 0, ENC_UNENCRYPTED_BUFFER_SIZE);
}

void PassThruEncryptor::encrypt(const char* plainText, int len, int slot) {
  // clear any buffers, initialize encryption algo
  prepareForEncryption();

  if (len < ENC_UNENCRYPTED_BUFFER_SIZE) {
    // copy the value to the encrypted and unencrypted buffer
    memcpy(this->unencryptedBuffer, plainText, len); 
    memcpy(this->encryptedBuffer, plainText, len); 
  } else {
    logConsole("ERROR: cleartext too large for unencrypted buffer");
  }
}

void PassThruEncryptor::encryptVolatile(const char* plainText, int len) {
  // clear any buffers, initialize encryption algo
  prepareForVolatileEncryption();

  if (len < ENC_UNENCRYPTED_BUFFER_SIZE) {
    // copy the value to the encrypted buffer
    memcpy(this->unencryptedBuffer, plainText, len); 
    memcpy(this->encryptedBuffer, plainText, len); 
  } else {
    logConsole("ERROR: cleartext too large for unencrypted buffer!");
  }
}

void PassThruEncryptor::decrypt(uint8_t* encrypted, int len, int slot) {
  // clear any buffers, initialize encryption algo
  prepareForEncryption();

  if (len < ENC_ENCRYPTED_BUFFER_SIZE) {
    // copy encrypted value into encrypted buffer
    memcpy(this->encryptedBuffer, encrypted, len); 
    memcpy(this->unencryptedBuffer, encrypted, len); 
  } else {
    logConsole("ERROR: encrypted text too large for buffer!");
  }
}

void PassThruEncryptor::decryptVolatile(uint8_t* encrypted, int len) {
  // clear any buffers, initialize encryption algo
  prepareForVolatileEncryption();

  if (len < ENC_ENCRYPTED_BUFFER_SIZE) {
    // copy encrypted value into encrypted buffer
    memcpy(this->encryptedBuffer, encrypted, len); 
    memcpy(this->unencryptedBuffer, encrypted, len); 
  } else {
    logConsole("ERROR: encrypted text too large for buffer!");
  }
}

