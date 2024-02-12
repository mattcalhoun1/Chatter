#include "SDTrustStore.h"

bool SDTrustStore::init () {
    bool success = true;
    if (!running) {
        // make sure structure exists
        if (!SD.exists(publicKeysDir)){
            success = SD.mkdir(publicKeysDir);
        }

        running = true;
    }
    return success;
}

void SDTrustStore::populateFullFileName(const char* deviceId) {
  memset(fullFileName, 0, STORAGE_MAX_TRUSTSTORE_FILENAME_LENGTH);
  memcpy(fullFileName, publicKeysDir, strlen(publicKeysDir));
  memcpy(fullFileName + strlen(publicKeysDir), deviceId, STORAGE_DEVICE_ID_LENGTH);  
}

List<String> SDTrustStore::getDeviceIds() {
  List<String> deviceIds;

  logConsole("Loading device IDs...");
  File dir = SD.open(publicKeysDir);
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }

    const char* fileName = entry.name();
    char knownDeviceName[STORAGE_DEVICE_ID_LENGTH + 1];// = char[STORAGE_DEVICE_ID_LENGTH + 1];
    memset(knownDeviceName, 0, STORAGE_DEVICE_ID_LENGTH + 1);
    memcpy(knownDeviceName, fileName, STORAGE_DEVICE_ID_LENGTH);
    //knownDeviceName[STORAGE_DEVICE_ID_LENGTH] = '\0';
    deviceIds.add(String(knownDeviceName));
    entry.close();

    //logConsole("Added device: " + String(knownDeviceName));
  }
  dir.close();
  //logConsole("Loaded " + String(deviceIds.getSize()) + " device IDs.");
  return deviceIds;    
}


bool SDTrustStore::findDeviceId (const char* key, char* deviceIdBuffer) {
  // look for that key in the truststore, populate matching device id (if any).
  // return true if a match is found
  bool found = false;
  char knownDeviceName[STORAGE_DEVICE_ID_LENGTH + 1];

  File dir = SD.open(publicKeysDir);
  while (!found) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }

    const char* fileName = entry.name();
    bool keyMatches = true;
    memset(knownDeviceName, 0, STORAGE_DEVICE_ID_LENGTH + 1);
    memcpy(knownDeviceName, fileName, STORAGE_DEVICE_ID_LENGTH);
    for (int i = 0; i < STORAGE_PUBLIC_KEY_LENGTH && keyMatches; i++) {
      if (entry.read() != key[i]) {
        keyMatches = false;
      }
    }
    entry.close();
    if (keyMatches) {
      memset(deviceIdBuffer, 0, STORAGE_DEVICE_ID_LENGTH + 1);
      memcpy(deviceIdBuffer, knownDeviceName, STORAGE_DEVICE_ID_LENGTH);
      found = true;
    }
  }
  dir.close();
  return found;
}

bool SDTrustStore::removeTrustedDevice (const char* deviceId) {
  if (isSafeFilename(deviceId) && strlen(deviceId) == STORAGE_DEVICE_ID_LENGTH) {
    logConsole("Removing public key for: " + String(deviceId));

    // If the device already exists on the sd card, delete it
    populateFullFileName(deviceId);
    if (SD.exists(fullFileName)) {
      return SD.remove(fullFileName);
    }
    else {
      logConsole("failed to remove found for: " + String(deviceId) + " !");
    }
  } else {
    logConsole("Bad device ID!");
  }
  return false;
}

bool SDTrustStore::clearTruststore () {
  logConsole("Clearing truststore...");
  File dir = SD.open(publicKeysDir);
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    const char* deviceId = entry.name();
    populateFullFileName(deviceId);
    entry.close();
    if (SD.remove(fullFileName)) {
      logConsole("Untrusted device: " + String(deviceId));
    }
    else {
      logConsole("Failed to untrust: " + String(fullFileName));
    }
  }
  dir.close();

}


bool SDTrustStore::loadPublicKey(const char* deviceId, char* keyBuffer) {
  if (isSafeFilename(deviceId) && strlen(deviceId) == STORAGE_DEVICE_ID_LENGTH) {
    logConsole("Reading public key for: " + String(deviceId));

    // If the device already exists on the sd card, delete it
    populateFullFileName(deviceId);

    if (SD.exists(fullFileName)) {
      File deviceFile = SD.open(fullFileName, FILE_READ);
      deviceFile.read(keyBuffer, STORAGE_PUBLIC_KEY_LENGTH);
      keyBuffer[STORAGE_PUBLIC_KEY_LENGTH] = '\0';
      deviceFile.close();
      return true;
    }
    else {
      logConsole("Public key not found for: " + String(deviceId) + " !");
    }
  } else {
    logConsole("Bad device ID!");
  }
  return false;
}

bool SDTrustStore::loadAlias(const char* deviceId, char* aliasBuffer) {
  if (isSafeFilename(deviceId) && strlen(deviceId) == STORAGE_DEVICE_ID_LENGTH) {
    //logConsole("Reading Alias for: " + String(deviceId));
    populateFullFileName(deviceId);

    if (SD.exists(fullFileName)) {
      File deviceFile = SD.open(fullFileName, FILE_READ);

      // skip ahead to alias
      deviceFile.seek(STORAGE_PUBLIC_KEY_LENGTH + 1);

      // read until next newline
      int nextChar = 0;
      int a = 0;
      while ((char)nextChar != '\n' && deviceFile.available() > 0) {
        nextChar = deviceFile.read();
        if ((char)nextChar == '\n') {
          aliasBuffer[a] = '\0';
        }
        else {
          aliasBuffer[a++] = (char)nextChar;
        }
      }
      deviceFile.close();
      return true;
    }
    else {
      logConsole("Alias not found for " + String(deviceId) + "!");
    }
  } else {
    logConsole("Bad device ID!");
  }
  return false;
}

bool SDTrustStore::addTrustedDevice (const char* deviceId, const char* alias, const char* publicKey) {
  return addTrustedDevice (deviceId, alias, publicKey, false);
}

bool SDTrustStore::addTrustedDevice (const char* deviceId, const char* alias, const char* publicKey, bool overwrite) {
  if (isSafeFilename(deviceId) && strlen(deviceId) == STORAGE_DEVICE_ID_LENGTH) {
    logConsole("Adding trusted device: " + String(deviceId));

    // If the device already exists on the sd card, delete it
    populateFullFileName(deviceId);

    if (SD.exists(fullFileName)) {
      if (overwrite) {
        logConsole("Existing device config will be overwritten.");
        SD.remove(fullFileName);
      }
      else {
        logConsole("Device already trusted. Not overwriting.");
      }
    }

    // Create a new file named the device ID
    // line 1 is public key
    // line 2 is alias
    File deviceFile = SD.open(fullFileName, FILE_WRITE);
    writeLineToFile(&deviceFile, publicKey);
    writeLineToFile(&deviceFile, alias);
    deviceFile.close();
  }
  else {
    logConsole("Bad device ID! Must be 6 digits, Only alphanumeric allowed");
  }
}

bool SDTrustStore::writeLineToFile (File* file, const char* content) {
    for (int charCount = 0; charCount < strlen(content); charCount++) {
      file->write((uint8_t)content[charCount]);
    }
    file->write((uint8_t)'\n');
    file->flush();
}