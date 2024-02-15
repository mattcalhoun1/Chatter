#include "Encryptor.h"

bool Encryptor::init() {
    if (!hsm->init()) {
        logConsole("Failed to communicate with HSM!");
        return false;
    }

    if (!hsm->lockDevice(CHATTER_SIGN_PK_SLOT, CHATTER_STORAGE_PK_SLOT)) {
        logConsole("HSM Not Locked!");
        return false;
    }

    syncKeys();

    bool hsmInitialized = false;
    if(loadDataSlot(DEVICE_ID_SLOT)) {
        memset(deviceId, 0, ENC_DATA_SLOT_SIZE+1);
        getTextSlotBuffer(deviceId);
        deviceId[CHATTER_DEVICE_ID_SIZE] = 0;
        if (strlen(deviceId) == 0) {
            logConsole("HSM not initialized (no device ID). Need to onboard");
            return false;
        }
        else {
            logConsole("Device ID: " + String(deviceId));
        }
    }
    else {
        logConsole("Device ID not set on encryption chip!!");
        deviceId[0] = '\0';
        return false;
    }


    if(loadEncryptionKey (ENCRYPTION_KEY_SLOT)) {
        generateNextVolatileKey();

        algo = new ChaChaAlgo(this->encryptionKeyBuffer, this->volatileEncryptionKey);
        if(loadEncryptionKey (ENCRYPTION_IV_SLOT)) {
            algo->init(this->encryptionKeyBuffer);
        }
        else {
            logConsole("Error loading encryption iv");
            return false;
        }
    }
    else {
        logConsole("Error loading symmetric key");
        return true;
    }

    return true;  
}

void Encryptor::generateNextVolatileKey() {
  int generatedBytes = 0;
  while (generatedBytes < ENC_VOLATILE_KEY_SIZE) {
    volatileEncryptionKey[generatedBytes++] = (uint8_t)getRandom();
  }
  volatileEncryptionKey[generatedBytes] = 0;
}

const char* Encryptor::getDeviceId() {
  return deviceId;
}

void Encryptor::syncKeys () {
  // insert setup code here if necessary
  // public keys:
  /*
  const char* pk_00 = "252923C6847099562B67045A2BACF1FFEB05F784A2E7AF40E5A6B03C074DA1221344369E32EB15BFB96760585856E22A41238E81CC40B262B8C4384F608DB416";
  trustStore->addTrustedDevice("USCAL000", "Repeater", pk_00, true);

  const char* pk_01 = "35BEC6DA10CD443B8F1F904AB27BCEA7C2B1ED2769DE7F45BD15D7B18234FB0C9761DE90D74F7C6C3403C02C3A51637EF78D4A9D369EE76858B3DFE0F5364A62";
  trustStore->addTrustedDevice("USCAL001", "Matt", pk_01, true);

  const char* pk_02 = "15011530882A2D765557D417BF5E1EC6CBA55DEFA8DC72778FA6608FFF2A3DEA77D3B6FEDF6B949C1E2D6851E3A859AAED625178180A7B72165C13B9BE76008D";
  trustStore->addTrustedDevice("USCAL200", "Base_0", pk_02);

  const char* pk_03 = "38A1FB9F68FA1CAFA563084CE9C3FB4AE96DA5D024E6A7D987D4206281EBE13CF6FA45ABECB399BA023DFBA9024F435D80DC7B7C162710FF7006A759F35BAF1D";
  trustStore->addTrustedDevice("USCAL201", "Base_1", pk_03);
  */
}

// encrypt the value, place cleartext in encrypted buffer
void Encryptor::encrypt(const char* plainText, int len) {
    algo->prepareForEncryption(this->unencryptedBuffer, ENC_UNENCRYPTED_BUFFER_SIZE, this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
    if (len < ENC_UNENCRYPTED_BUFFER_SIZE) {
        // copy the value to the encrypted buffer
        memcpy(this->unencryptedBuffer, plainText, len); 

        // encrypt the buffer
        algo->encrypt(this->unencryptedBuffer, len, this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
    } else {
        logConsole("ERROR: cleartext too large for unencrypted buffer!");
    }
}
void Encryptor::encryptVolatile(const char* plainText, int len) {
    algo->prepareForVolatileEncryption(this->unencryptedBuffer, ENC_UNENCRYPTED_BUFFER_SIZE, this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
    if (len < ENC_UNENCRYPTED_BUFFER_SIZE) {
        // copy the value to the encrypted buffer
        memcpy(this->unencryptedBuffer, plainText, len); 

        // encrypt the buffer
        algo->encryptVolatile(this->unencryptedBuffer, len, this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
    } else {
        logConsole("ERROR: cleartext too large for unencrypted buffer!");
    }

}

// decrypt the value, place cleartext in unencrypted buffer
void Encryptor::decrypt(uint8_t* encrypted, int len) {
    algo->prepareForEncryption(this->unencryptedBuffer, ENC_UNENCRYPTED_BUFFER_SIZE, this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);

    if (len < ENC_ENCRYPTED_BUFFER_SIZE) {
        // copy encrypted value into encrypted buffer
        memcpy(this->encryptedBuffer, encrypted, len); 

        // decrypt buffer
        algo->decrypt(this->encryptedBuffer, len, this->unencryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
    } else {
        logConsole("ERROR: clear text too large for buffer!");
    }
}

void Encryptor::decryptVolatile(uint8_t* encrypted, int len) {
    algo->prepareForVolatileEncryption(this->unencryptedBuffer, ENC_UNENCRYPTED_BUFFER_SIZE, this->encryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);

    if (len < ENC_ENCRYPTED_BUFFER_SIZE) {
        // copy encrypted value into encrypted buffer
        memcpy(this->encryptedBuffer, encrypted, len); 

        // decrypt buffer
        algo->decryptVolatile(this->encryptedBuffer, len, this->unencryptedBuffer, ENC_ENCRYPTED_BUFFER_SIZE);
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

void Encryptor::setTextSlotBuffer (const char* textData) {
  memset(hexBuffer, 0, ENC_DATA_SLOT_SIZE);
  hexify ((const byte*)textData, strlen(textData));
  setDataSlotBuffer(hexBuffer);
}

int Encryptor::getTextSlotBuffer (char* target) {
  int textLen;
  for (textLen = 0; textLen < ENC_DATA_SLOT_SIZE && dataSlotBuffer[textLen] != 0 && dataSlotBuffer[textLen] != 255; textLen++) {
    target[textLen] = (char)dataSlotBuffer[textLen];
  }

  // copy null terms to the rest
  memset(target+textLen, 0, ENC_DATA_SLOT_SIZE - textLen);
  return strlen(target);
}


void Encryptor::setDataSlotBuffer (const char* hexData) {
    if (strlen(hexData) > ENC_DATA_SLOT_SIZE) {
        logConsole("ERROR: setDataSlotBuffer hex data too large for slot!");
        return;
    }
    hexCharacterStringToBytes(dataSlotBuffer, hexData, ENC_DATA_SLOT_SIZE);
}

void Encryptor::setDataSlotBuffer (byte* data) {
  for (int bCount = 0; bCount < ENC_DATA_SLOT_SIZE; bCount++) {
    this->dataSlotBuffer[bCount] = dataSlotBuffer[bCount];
  }
}

byte* Encryptor::getDataSlotBuffer () {
  return this->dataSlotBuffer;
}

void Encryptor::setSignatureBuffer(byte *signatureBuffer) {
  for (int bCount = 0; bCount < ENC_SIG_BUFFER_SIZE; bCount++) {
    this->signatureBuffer[bCount] = signatureBuffer[bCount];
  }
}

byte* Encryptor::getSignatureBuffer () {
  return signatureBuffer;
}

bool Encryptor::signMessage (int slot) {
  // Sign whatever is in the message buffer, storing signature in sig buffer
  return hsm->sign(slot, messageBuffer, signatureBuffer);
}

int Encryptor::generateHash(const char* plainText, int inputLength, uint8_t* hashBuffer) {
  SHA256 hasher;
  hasher.update(plainText, inputLength);
  hasher.finalize(hashBuffer, hasher.hashSize());
  return hasher.hashSize();
}

bool Encryptor::setPublicKeyBuffer (const char* publicKey) {
  hexCharacterStringToBytes(publicKeyBuffer, publicKey, ENC_PUB_KEY_SIZE);
}

bool Encryptor::loadEncryptionKey (int slot) {
    if (loadedEncryptionKeySlot == slot) {
        return true;
    }

    logConsole("Loading encryption key from slot " + String(slot));

    // load that slot into the buffer
    if(this->loadDataSlot(slot)) {
        hexify(this->dataSlotBuffer, ENC_SYMMETRIC_KEY_SIZE);

        // copy over to the key buffer as uint8_t instead of byte
        for (int i = 0; i < ENC_SYMMETRIC_KEY_SIZE; i++) {
            this->encryptionKeyBuffer[i] = (uint8_t)(this->hexBuffer[i]);
        }
        loadedEncryptionKeySlot = slot;
        return true;
    }

    logConsole("unable to load encryption key");
    return false;
}

bool Encryptor::loadPublicKey(int slot) {
  return hsm->loadPublicKey(slot, publicKeyBuffer);
}

byte* Encryptor::getPublicKeyBuffer() {
  return this->publicKeyBuffer;
}

bool Encryptor::verify() {
  return hsm->verifySignature(this->getMessageBuffer(), this->getSignatureBuffer(), this->publicKeyBuffer);
}

bool Encryptor::verify(int slot) {
  if (loadPublicKey(slot)) {
    return hsm->verifySignature(this->getMessageBuffer(), this->getSignatureBuffer(), this->publicKeyBuffer);
  }
  else {
    logConsole("Load public key failed during sig verify");
    return false;
  }
}

bool Encryptor::verify(const byte pubkey[]) {
    return hsm->verifySignature(this->getMessageBuffer(), this->getSignatureBuffer(), pubkey);
}

bool Encryptor::loadDataSlot(int slot) {
  // if already loaded, skip it
  if (loadedDataSlot == slot) {
    return true;
  }

  memset(dataSlotBuffer, 0, ENC_DATA_SLOT_SIZE);
  if (hsm->readSlot(slot, dataSlotBuffer, ENC_DATA_SLOT_SIZE)) {
    loadedDataSlot = slot;
    return true;
  }
  else {
    loadedDataSlot = -1;
    logConsole("Load data slot failed");
    return false;
  }
}

bool Encryptor::saveDataSlot(int slot) {
  loadedDataSlot = -1;
  if (hsm->writeSlot(slot, dataSlotBuffer, ENC_DATA_SLOT_SIZE)) {
    return true;
  }
  else {
    logConsole("Save data slot failed");
    return false;
  }
}

int Encryptor::findEncryptionBufferEnd (uint8_t* buffer, int maxLen) {
  for (int i = 0; i < maxLen - 3; i++) {
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
  for (int i = 0; i < maxLen; i++) {
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

void Encryptor::hexify (const byte input[], int inputLength) {
  static const char characters[] = "0123456789ABCDEF";
  char* tmpHexBuffer = hexBuffer;
  
  for (int i = 0; i < inputLength; i++) {
    byte oneInputByte = input[i];
    *tmpHexBuffer++ = characters[oneInputByte >> 4];
    *tmpHexBuffer++ = characters[oneInputByte & 0x0F];
  }
  *tmpHexBuffer = '\0';// terminate c string
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
