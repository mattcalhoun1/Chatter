#include "Encryptor.h"

bool Encryptor::init() {
  if (!ECCX08.begin()) {
    logConsole("Failed to communicate with ECC508/ECC608!");
    return false;
  }

  if (!lockEncryptionDevice()) {
    logConsole("The ECC508/ECC608 is not locked!");
    return false;
  }

  syncKeys();

  if(loadDataSlot(DEVICE_ID_SLOT)) {
    getTextSlotBuffer(deviceId);
    logConsole("Device ID: " + String(deviceId));
  }
  else {
    logConsole("Device ID not set on encryption chip!!");
    deviceId[0] = '\0';
    return false;
  }

  // TEMP
  /*memcpy(deviceId, "USCAL001", 8);
  deviceId[8] = '\0';
  setTextSlotBuffer(deviceId);
  saveDataSlot(DEVICE_ID_SLOT);*/

  if (memcmp(deviceId, "US", 2) != 0) {
    logConsole("Updating to 8 digit device id");
    for (int i = 7; i > 1; i--) {
      deviceId[i] = deviceId[i - 2];
    }
    deviceId[0] = 'U';
    deviceId[1] = 'S';
    deviceId[8] = '\0';
    setTextSlotBuffer(deviceId);
    saveDataSlot(DEVICE_ID_SLOT);    
  }

  generateNextVolatileKey();

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
  const char* pk_00 = "252923C6847099562B67045A2BACF1FFEB05F784A2E7AF40E5A6B03C074DA1221344369E32EB15BFB96760585856E22A41238E81CC40B262B8C4384F608DB416";
  trustStore->addTrustedDevice("USCAL000", "Repeater", pk_00, true);

  const char* pk_01 = "35BEC6DA10CD443B8F1F904AB27BCEA7C2B1ED2769DE7F45BD15D7B18234FB0C9761DE90D74F7C6C3403C02C3A51637EF78D4A9D369EE76858B3DFE0F5364A62";
  trustStore->addTrustedDevice("USCAL001", "Matt", pk_01, true);

  const char* pk_02 = "15011530882A2D765557D417BF5E1EC6CBA55DEFA8DC72778FA6608FFF2A3DEA77D3B6FEDF6B949C1E2D6851E3A859AAED625178180A7B72165C13B9BE76008D";
  trustStore->addTrustedDevice("USCAL200", "Base_0", pk_02);

  const char* pk_03 = "38A1FB9F68FA1CAFA563084CE9C3FB4AE96DA5D024E6A7D987D4206281EBE13CF6FA45ABECB399BA023DFBA9024F435D80DC7B7C162710FF7006A759F35BAF1D";
  trustStore->addTrustedDevice("USCAL201", "Base_1", pk_02);
}

long Encryptor::getRandom() {
  return ECCX08.random(65535);
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
  hexCharacterStringToBytes(dataSlotBuffer, hexData);
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

int Encryptor::sign (int slot) {
  // Sign whatever is in the message buffer, storing signature in sig buffer
  return ECCX08.ecSign(slot, messageBuffer, signatureBuffer) == 0;
}

int Encryptor::generateHash(const char* plainText, int inputLength, uint8_t* hashBuffer) {
  SHA256 hasher;
  hasher.update(plainText, inputLength);
  hasher.finalize(hashBuffer, hasher.hashSize());
  return hasher.hashSize();
}

bool Encryptor::setPublicKeyBuffer (const char* publicKey) {
  hexCharacterStringToBytes(publicKeyBuffer, publicKey);
}

void Encryptor::loadEncryptionKey (int slot) {
  if (loadedEncryptionKeySlot == slot) {
    return;
  }

  logConsole("Loading encryption key from slot " + String(slot));

  // load that slot into the buffer
  this->loadDataSlot(slot);

  hexify(this->dataSlotBuffer, ENC_SYMMETRIC_KEY_SIZE);

  // copy over to the key buffer as uint8_t instead of byte
  for (int i = 0; i < ENC_SYMMETRIC_KEY_SIZE; i++) {
    this->encryptionKeyBuffer[i] = (uint8_t)(this->hexBuffer[i]);
  }
  loadedEncryptionKeySlot = slot;
}

bool Encryptor::lockEncryptionDevice () {
  if (!ECCX08.locked()) {
    if (!ECCX08.writeConfiguration(ECCX08_DEFAULT_TLS_CONFIG)) {
      logConsole("Writing default config to encryption device failed");
      return false;
    }

    if (!ECCX08.lock()) {
      logConsole("Locking ECCX08 configuration failed!");
      return false;
    }

    logConsole("Encryption device successfully locked");

    // since this device was just locked, generate the initial keypair
    generateNewKeypair(CHATTER_SIGN_PK_SLOT, CHATTER_STORAGE_PK_SLOT, CHATTER_SSC_YEAR, CHATTER_SSC_MONTH, CHATTER_SSC_DATE, CHATTER_SSC_HOUR, CHATTER_SSC_VALID);
  }

  return true;
}

bool Encryptor::generateNewKeypair (int pkSlot, int pkStorage, int year, int month, int day, int hour, int expire) {
  if (!ECCX08SelfSignedCert.beginStorage(pkSlot, pkStorage, true)) {
    logConsole("Error starting self signed cert generation!");
    return false;
  }

  // if rtc is available, use that instead
  ECCX08SelfSignedCert.setCommonName(ECCX08.serialNumber());
  ECCX08SelfSignedCert.setIssueYear(year);
  ECCX08SelfSignedCert.setIssueMonth(month);
  ECCX08SelfSignedCert.setIssueDay(day);
  ECCX08SelfSignedCert.setIssueHour(hour);
  ECCX08SelfSignedCert.setExpireYears(expire);

  String cert = ECCX08SelfSignedCert.endStorage();

  if (!cert) {
    logConsole("Error generating self signed cert!");
    return false;
  }

  logConsole("New Cert: ");
  logConsole(cert);

  logConsole("SHA1: ");
  logConsole(ECCX08SelfSignedCert.sha1());

  return true;
}

bool Encryptor::loadPublicKey(int slot) {

  int loadResult = ECCX08.generatePublicKey(slot, publicKeyBuffer);
  if (loadResult == 1) {
    // print the public key
    Serial.print("Public key of slot ");
    Serial.print(slot);
    Serial.print(" is:   ");
    logBufferHex(publicKeyBuffer, ENC_PUB_KEY_SIZE);
    return true;
  }
  else {
    logConsole("Pub key load returned error: " + String(loadResult));
  }
  return false;
}

byte* Encryptor::getPublicKeyBuffer() {
  return this->publicKeyBuffer;
}

bool Encryptor::verify() {
  return ECCX08.ecdsaVerify(this->getMessageBuffer(), this->getSignatureBuffer(), this->publicKeyBuffer);
}

bool Encryptor::verify(int slot) {
  if (loadPublicKey(slot)) {
    return ECCX08.ecdsaVerify(this->getMessageBuffer(), this->getSignatureBuffer(), this->publicKeyBuffer);
  }
  else {
    logConsole("Load public key failed during sig verify");
    return false;
  }
}

bool Encryptor::verify(const byte pubkey[]) {
  return ECCX08.ecdsaVerify(this->getMessageBuffer(), this->getSignatureBuffer(), pubkey);
}

bool Encryptor::loadDataSlot(int slot) {
  // if already loaded, skip it
  if (loadedDataSlot == slot) {
    return true;
  }

  int result = ECCX08.readSlot(slot, dataSlotBuffer, ENC_DATA_SLOT_SIZE);

  if (result == 1) {
    loadedDataSlot = slot;
    return true;
  }
  else {
    loadedDataSlot = -1;
    logConsole("Load data slot returned fail: " + String(result));
    return false;
  }
}

bool Encryptor::saveDataSlot(int slot) {
  loadedDataSlot = -1;
  int result = ECCX08.writeSlot(slot, dataSlotBuffer, ENC_DATA_SLOT_SIZE);
  if (result == 1) {
    return true;
  }
  else {
    logConsole("Save data slot returned fail: " + String(result));
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
  if (CHAT_LOG_ENABLED) {
    Serial.println(msg);
  }
}

void Encryptor::hexCharacterStringToBytes(byte *byteArray, const char *hexString)
{
  hexCharacterStringToBytes(byteArray, hexString, strlen(hexString));
}

void Encryptor::hexCharacterStringToBytes(byte *byteArray, const char *hexString, int hexLength) {
  bool oddLength = hexLength & 1;

  byte currentByte = 0;
  byte byteIndex = 0;

  for (byte charIndex = 0; charIndex < hexLength; charIndex++)
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
