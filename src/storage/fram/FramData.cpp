#include "FramData.h"

FramData::FramData (const uint8_t* _key, const uint8_t* _volatileKey, FramConnectionType _connectionType) {
  chacha.setKey(_key, ENC_SYMMETRIC_KEY_SIZE);
  chachaVolatile.setKey(_volatileKey, ENC_SYMMETRIC_KEY_SIZE);
  memcpy(iv, _key+ENC_SYMMETRIC_KEY_SIZE, ENC_IV_SIZE);
  connectionType = _connectionType;
}

FramData::FramData(const char* _passphrase, uint8_t _length, FramConnectionType _connectionType) {
    connectionType = _connectionType;

    // in this case, we generate our own key using the given passphrase
    uint8_t hashOfPassphrase[32];
    memset(hashOfPassphrase, 0, 32);

    SHA256 hasher;
    hasher.clear();
    hasher.update(_passphrase, _length);
    hasher.finalize(hashOfPassphrase, hasher.hashSize());

    // first 16 go to key
    chacha.setKey(hashOfPassphrase, ENC_SYMMETRIC_KEY_SIZE);

    // next 8 go to iv
    memcpy(iv, hashOfPassphrase+ENC_SYMMETRIC_KEY_SIZE, ENC_IV_SIZE);

    // now generate volatile key
    uint8_t volKey[ENC_SYMMETRIC_KEY_SIZE];

    for (uint8_t i = 0; i < ENC_SYMMETRIC_KEY_SIZE; i++) {
        randomSeed(millis());
        volKey[i] = random(255);
        delay(10); // sleep for a hopefully somewhat unpredictable amount of time
    }
    chachaVolatile.setKey(volKey, ENC_SYMMETRIC_KEY_SIZE);

}


bool FramData::init() {
    switch (connectionType) {
        case FramSPI:
            fram = new FramAdafruitSPI();
            break;
        case FramI2C:
            fram = new FramAdafruitI2C();
            break;
        default:
            Serial.println("Error: bad fram type");
            return false;
    }
    running = fram->init();

    // clear any volatile zones
    for (uint8_t i = 0; i < FRAM_NUM_ZONES; i++) {
    if (zoneVolatile[i]) {
        clearZone(zoneId[i]);
    }
    }

    return running;
}

bool FramData::recordExists (FramZone zone, uint8_t* recordKey) {
  return getRecordNum(zone, recordKey) != FRAM_NULL; // obviously we can't have 255 slots or this logic breaks
}


bool FramData::writeToNextSlot (FramRecord* record) {
  // get the next available slot
  uint8_t nextSlot = getNextSlot(record->getZone());
  if (writeRecord(record, nextSlot)) {
    // bump the last used slot to what we just used
    uint16_t zoneLoc = zoneLocations[record->getZone()];
    if(fram->write(zoneLoc + 1, nextSlot)) {
      uint8_t slotsUsed = FramData::getNumUsedSlots(record->getZone());

      if (slotsUsed < zoneSlots[record->getZone()]) {
        return fram->write(zoneLoc, slotsUsed+1);
      }

      return true;
    }
    else {
      Serial.println("Unable to update next slot");
    }

    return false;
  }

  Serial.println("record write failed");
  return false;
}


bool FramData::writeRecord (FramRecord* record, uint8_t slot) {
  // write the key
  record->serializeKey(unencryptedBuffer);
  uint16_t dataPosition = getFramKeyLocation(record->getZone(), slot);

  if(!fram->write(dataPosition, unencryptedBuffer, zoneKeySizes[record->getZone()])) {
    Serial.println("Key not written to fram");
  }

  // serialize into unencrypted byffer
  memset(unencryptedBuffer, 0, FRAM_ENC_BUFFER_SIZE);
  uint16_t actualDataSize = record->serialize(unencryptedBuffer);
  dataPosition = getFramDataLocation(record->getZone(), slot);

  // enrypt
  uint16_t bytesToWrite = actualDataSize;
  if (zoneEncrypted[record->getZone()]) {
    encrypt(unencryptedBuffer, actualDataSize);
    bytesToWrite = guessEncryptionBufferSize(encryptedBuffer, 0);
    fram->write(dataPosition, encryptedBuffer, bytesToWrite);
  }
  else {
    fram->write(dataPosition, unencryptedBuffer, bytesToWrite);
  }

  return true;
}

bool FramData::readRecord (FramRecord* record, uint8_t slot) {
  // read the key
  //uint8_t recordKey[FRAM_PACKET_KEYSIZE];
  memset(keyBuffer, 0, FRAM_MAX_KEYSIZE); // for temporarily holding keys, various purposes. choosing largest key size

  uint16_t dataPosition = getFramKeyLocation(record->getZone(), slot);
  fram->read(dataPosition, keyBuffer, zoneKeySizes[record->getZone()]);

    dataPosition = getFramDataLocation(record->getZone(), slot);
    memset(unencryptedBuffer, 0, FRAM_ENC_BUFFER_SIZE);

  if (zoneEncrypted[record->getZone()]) {
    // read into the encrypted buffer
    memset(encryptedBuffer, 0, zoneDataSizes[record->getZone()]);
    fram->read(dataPosition, encryptedBuffer, zoneDataSizes[record->getZone()]);
    decrypt(encryptedBuffer, FRAM_ENC_BUFFER_SIZE);
  }
  else {
    // write the encrypted record to the appropriate slot
    fram->read(dataPosition, unencryptedBuffer, zoneDataSizes[record->getZone()]);
  }

  // ingest the record
  record->deserialize(keyBuffer, unencryptedBuffer);
  return true;
}

uint16_t FramData::getFramDataLocation (FramZone zone, uint8_t slot) {
  // get the key position
  uint16_t dataLocation = getFramKeyLocation(zone, slot);

  // jump past the key
  dataLocation += zoneKeySizes[zone];

  return dataLocation;
}

uint16_t FramData::getFramKeyLocation (FramZone zone, uint8_t slot) {
  uint16_t keyPosition = zoneLocations[zone] + FRAM_ZONE_METADATA_SIZE;

  // now we are pointed at the first key
  keyPosition += (slot * (zoneKeySizes[zone] + zoneDataSizes[zone]));

  return keyPosition;
}

uint8_t FramData::getNumUsedSlots (FramZone zone) {
  uint8_t used = fram->read(zoneLocations[zone]);
  return used;
}

uint8_t FramData::getNextSlot (FramZone zone) {
  uint8_t newestSlot = fram->read(zoneLocations[zone] + 1);
  if (newestSlot >= zoneSlots[zone] - 1) {
    return 0;
  }
  return newestSlot + 1; // slot after newest will be next
}

// chacha encryption doens't return encrypted or decrypted length, so
// i use terminators to do a best guess
uint8_t FramData::guessEncryptionBufferSize (uint8_t* buffer, uint8_t terminator) {
    // find the index of at least 3 repeating terminators
    uint8_t size = 0;
    uint8_t seqLen = 0; // how many terms in a rawContentLength
    for (uint8_t i = 0; i < FRAM_ENC_BUFFER_SIZE; i++) {
        if (buffer[i] == terminator) {
            seqLen++;
        }
        else {
            seqLen = 0;
            size = i+1;
        }

        if (seqLen >= 3) {
            return size;
        }
    }
    return size;
}

void FramData::encrypt(uint8_t* clear, int clearLength) {
  memset(encryptedBuffer, 0, FRAM_ENC_BUFFER_SIZE);
  chacha.clear();
  chacha.setIV(iv, ENC_IV_SIZE);

  chacha.encrypt (encryptedBuffer, clear, clearLength);
}

void FramData::decrypt(uint8_t* encrypted, int encryptedLength) {
  memset(unencryptedBuffer, 0, FRAM_ENC_BUFFER_SIZE);
  chacha.clear();
  chacha.setIV(iv, ENC_IV_SIZE);
  chacha.decrypt (unencryptedBuffer, encrypted, encryptedLength);
}

void FramData::encryptVolatile(uint8_t* clear, int clearLength) {
  memset(encryptedBuffer, 0, FRAM_ENC_BUFFER_SIZE);
  chachaVolatile.clear();
  chachaVolatile.setIV(iv, ENC_IV_SIZE);

  chachaVolatile.encrypt (encryptedBuffer, clear, clearLength);
}

void FramData::decryptVolatile(uint8_t* encrypted, int encryptedLength) {
  memset(unencryptedBuffer, 0, FRAM_ENC_BUFFER_SIZE);
  chachaVolatile.clear();
  chachaVolatile.setIV(iv, ENC_IV_SIZE);
  chachaVolatile.decrypt (unencryptedBuffer, encrypted, encryptedLength);
}

bool FramData::clearAllZones () {
    bool allClear = true;
    for (uint8_t i = 0; i < FRAM_NUM_ZONES; i++) {
        allClear = allClear && clearZone(zoneId[i]);
    }
    return allClear;
}

bool FramData::clearZone (FramZone zone) {
  // set the metadata back to zeros so the entire zone becomes unused
  uint16_t addr = zoneLocations[zone];
  fram->write(addr, 0);

  // latest slot pointer goes to the last, so next will result in 0
  Serial.print("Clear zone: setting latest used slot for zone: ");
  Serial.print(zone);
  Serial.print(" to ");
  Serial.println(zoneSlots[zone] - 1);

  fram->write(addr+1, zoneSlots[zone] - 1);
}
