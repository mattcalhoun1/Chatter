#include "Encryptor.h"

bool Encryptor::init() {
    // hsm should already be initialized

    return true;  
}

// encrypt the value, place cleartext in encrypted buffer
void Encryptor::encrypt(const char* plainText, int len) {
    baseUnencryptedBufferLength = len;
    hsm->prepareForEncryption(this->unencryptedBuffer, ENC_UNENCRYPTED_BUFFER_SIZE, this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
    if (len < ENC_UNENCRYPTED_BUFFER_SIZE) {
        // copy the value to the encrypted buffer
        memcpy(this->unencryptedBuffer, plainText, len); 

        // encrypt the buffer
        hsm->encrypt(this->unencryptedBuffer, len, this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
    } else {
        logConsole("ERROR: cleartext too large for unencrypted buffer!");
    }
}
void Encryptor::encryptVolatile(const char* plainText, int len) {
    baseUnencryptedBufferLength = len;
    hsm->prepareForVolatileEncryption(this->unencryptedBuffer, ENC_UNENCRYPTED_BUFFER_SIZE, this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
    if (len < ENC_UNENCRYPTED_BUFFER_SIZE) {
        // copy the value to the encrypted buffer
        memcpy(this->unencryptedBuffer, plainText, len); 

        // encrypt the buffer
        hsm->encryptVolatile(this->unencryptedBuffer, len, this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
    } else {
        logConsole("ERROR: cleartext too large for unencrypted buffer!");
    }

}

// decrypt the value, place cleartext in unencrypted buffer
void Encryptor::decrypt(uint8_t* encrypted, int len) {
    baseEncryptedBufferLength = len;
    hsm->prepareForEncryption(this->unencryptedBuffer, ENC_UNENCRYPTED_BUFFER_SIZE, this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);

    if (len < ENC_ENCRYPTED_BUFFER_SIZE) {
        // copy encrypted value into encrypted buffer
        memcpy(this->encryptedBuffer, encrypted, len); 

        // decrypt buffer
        hsm->decrypt(this->encryptedBuffer, len, this->unencryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
    } else {
        logConsole("ERROR: clear text too large for buffer!");
    }
}

void Encryptor::decryptVolatile(uint8_t* encrypted, int len) {
    baseEncryptedBufferLength = len;
    hsm->prepareForVolatileEncryption(this->unencryptedBuffer, ENC_UNENCRYPTED_BUFFER_SIZE, this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);

    if (len < ENC_ENCRYPTED_BUFFER_SIZE) {
        // copy encrypted value into encrypted buffer
        memcpy(this->encryptedBuffer, encrypted, len); 

        // decrypt buffer
        hsm->decryptVolatile(this->encryptedBuffer, len, this->unencryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
    } else {
        logConsole("ERROR: clear text too large for buffer!");
    }

}

long Encryptor::getRandom() {
  return hsm->getRandomLong();
}

void Encryptor::setMessageBuffer(byte *messageBuffer) {
  for (int bCount = 0; bCount < ENC_MSG_BUFFER_SIZE; bCount++) {
    this->messageBuffer[bCount] = messageBuffer[bCount];
  }
}

byte* Encryptor::getMessageBuffer () {
  return messageBuffer;
}

void Encryptor::setSignatureBuffer(byte *signatureBuffer) {
  for (int bCount = 0; bCount < ENC_SIG_BUFFER_SIZE; bCount++) {
    this->signatureBuffer[bCount] = signatureBuffer[bCount];
  }
}

byte* Encryptor::getSignatureBuffer () {
  return signatureBuffer;
}

bool Encryptor::signMessage () {
  // Sign whatever is in the message buffer, storing signature in sig buffer
  return hsm->sign(messageBuffer, signatureBuffer);
}

int Encryptor::generateHash(const char* plainText, int inputLength, uint8_t* hashBuffer) {
  SHA256 hasher;
  hasher.update(plainText, inputLength);
  hasher.finalize(hashBuffer, hasher.hashSize());
  return hasher.hashSize();
}

bool Encryptor::setPublicKeyBuffer (uint8_t* publicKey) {
    memcpy(publicKeyBuffer, publicKey, ENC_PUB_KEY_SIZE);
}

bool Encryptor::loadPublicKey() {
  return hsm->loadPublicKey(publicKeyBuffer);
}

byte* Encryptor::getPublicKeyBuffer() {
  return this->publicKeyBuffer;
}

bool Encryptor::verify() {
  return hsm->verifySignature(this->getMessageBuffer(), this->getSignatureBuffer(), this->publicKeyBuffer);
}

bool Encryptor::verify(const byte pubkey[]) {
    return hsm->verifySignature(this->getMessageBuffer(), this->getSignatureBuffer(), pubkey);
}

int Encryptor::findEncryptionBufferEnd (uint8_t* buffer, int maxLen) {
    // then encryption buffer is at least the size of unencrypted buffer
  for (int i = baseUnencryptedBufferLength; i < maxLen - 3; i++) {
    if (buffer[i] == 255 && buffer[i+1] == 255 && buffer[i+1] == 255) {
      return i;
    }
    else if (buffer[i] == 0 && buffer[i+1] == 0 && buffer[i+1] == 0) {
      return i;
    }
  }
  return maxLen;
}

int Encryptor::findDecryptionBufferEnd (uint8_t* buffer, int maxLen) {
    // the decrypted length is at least half the size of the encrypted buffer it was based on
  for (int i = (baseEncryptedBufferLength / 2); i < maxLen; i++) {
    if (buffer[i] == 255 && buffer[i+1] == 255 && buffer[i+1] == 255) {
      return i;
    }
    else if (buffer[i] == 0 && buffer[i+1] == 0 && buffer[i+1] == 0) {
      return i;
    }
  }
  return maxLen;
}

uint8_t* Encryptor::getEncryptedBuffer() {
  return this->encryptedBuffer;
}

uint8_t* Encryptor::getUnencryptedBuffer() {
  return this->unencryptedBuffer;
}

int Encryptor::getEncryptedBufferLength() {
  return findEncryptionBufferEnd(this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
}

int Encryptor::getUnencryptedBufferLength() {
  return findDecryptionBufferEnd(this->unencryptedBuffer, ENC_UNENCRYPTED_BUFFER_SIZE);
}


const char* Encryptor::getHexBuffer () {
  return this->hexBuffer;
}

void Encryptor::hexify (char* outputBuffer, const byte input[], int inputLength) {
  static const char characters[] = "0123456789ABCDEF";
  char* tmpHexBuffer = outputBuffer;

    int totalBytes = 0;
  for (int i = 0; i < inputLength; i++) {
    byte oneInputByte = input[i];
    *tmpHexBuffer++ = characters[oneInputByte >> 4];
    *tmpHexBuffer++ = characters[oneInputByte & 0x0F];
    totalBytes += 2;
  }
  *tmpHexBuffer = '\0';// terminate c string
}

void Encryptor::hexify (const byte input[], int inputLength) {
    Encryptor::hexify(hexBuffer, input, inputLength);
}

void Encryptor::clearHexBuffer () {
    memset(hexBuffer, 0, ENC_HEX_BUFFER_SIZE);
}

void Encryptor::logBufferHex(const byte input[], int inputLength) {
  for (int i = 0; i < inputLength; i++) {
    Serial.print(input[i] >> 4, HEX);
    Serial.print(input[i] & 0x0f, HEX);
  }
  Serial.println();
}

void Encryptor::logConsole(String msg) {
  if (ENC_LOG_ENABLED) {
    Serial.println(msg);
  }
}

void Encryptor::hexCharacterStringToBytes(byte *byteArray, const char *hexString, int maxBufferSize)
{
  hexCharacterStringToBytesMax(byteArray, hexString, strlen(hexString), maxBufferSize);
}

void Encryptor::hexCharacterStringToBytesMax(byte *byteArray, const char *hexString, int hexLength, int maxBufferSize) {
    bool oddLength = hexLength & 1;

    byte currentByte = 0;
    byte byteIndex = 0;
    byte charIndex = 0;
  for (charIndex = 0; charIndex < hexLength && byteIndex < maxBufferSize; charIndex++)
  {
    bool oddCharIndex = charIndex & 1;

    if (oddLength)
    {
      // If the length is odd
      if (oddCharIndex)
      {
        // odd characters go in high nibble
        currentByte = nibble(hexString[charIndex]) << 4;
      }
      else
      {
        // Even characters go into low nibble
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
    else
    {
      // If the length is even
      if (!oddCharIndex)
      {
        // Odd characters go into the high nibble
        currentByte = nibble(hexString[charIndex]) << 4;
      }
      else
      {
        // Odd characters go into low nibble
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
  }

  if (byteIndex >= maxBufferSize && charIndex < hexLength) {
    logConsole("Error buffer overflow hexCharacterStringToBytesMax");
  }
}


byte Encryptor::nibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0;  // Not a valid hexadecimal character
}
