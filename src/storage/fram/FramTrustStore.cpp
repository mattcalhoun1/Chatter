#include "FramTrustStore.h"

bool FramTrustStore::init () {

    // add default device (temp)
    addTrustedDevice("USCAL000", "Bridge_Lora", "38A1FB9F68FA1CAFA563084CE9C3FB4AE96DA5D024E6A7D987D4206281EBE13CF6FA45ABECB399BA023DFBA9024F435D80DC7B7C162710FF7006A759F35BAF1D");

    return true;
}

void FramTrustStore::populateKeyBuffer(const char* deviceId) {
    // search for active devices only
    memcpy(keyBuffer, deviceId, STORAGE_DEVICE_ID_LENGTH);
    keyBuffer[STORAGE_DEVICE_ID_LENGTH] = TrustActive;
}

List<String> FramTrustStore::getDeviceIds() {
    List<String> deviceIds;

    logConsole("Loading device IDs...");
    uint8_t used = datastore->getNumUsedSlots(ZoneTrust);
    for (uint8_t slot = 0; slot < used; slot++) {
        datastore->readKey(keyBuffer, ZoneTrust, slot);

        // first 8 chars are the device id. last is the status
        keyBuffer[STORAGE_DEVICE_ID_LENGTH] = 0;
        deviceIds.add(String((char*)keyBuffer));
    }
    //logConsole("Loaded " + String(deviceIds.getSize()) + " device IDs.");
    return deviceIds;    
}


bool FramTrustStore::findDeviceId (const char* key, char* deviceIdBuffer) {
    // look for that key in the truststore, populate matching device id (if any).
    // return true if a match is found
    uint8_t used = datastore->getNumUsedSlots(ZoneTrust);
    for (int slot = 0; slot < used; slot++) {
        // read the key first, to make sure the device is active
        datastore->readKey(keyBuffer, ZoneTrust, slot);
        if (keyBuffer[STORAGE_DEVICE_ID_LENGTH] == TrustActive) {
            datastore->readRecord(&trustBuffer, slot);
            if (memcmp(deviceIdBuffer, trustBuffer.getPublicKey(), STORAGE_PUBLIC_KEY_LENGTH) == 0) {
                memset(deviceIdBuffer, 0, STORAGE_DEVICE_ID_LENGTH + 1);
                memcpy(deviceIdBuffer, trustBuffer.getDeviceId(), STORAGE_DEVICE_ID_LENGTH);
                return true;
            }
        }
    }

    return false;
}

// find lowest numeric gap in truststore so a new device can be onboarded
bool FramTrustStore::findNextAvailableDeviceId (const char* networkPrefix, int startingAddress, char* deviceIdBuffer) {

    // for now, number devices limited by 1) memory : map of device ids/keys, 2) fram size
    for (uint8_t deviceNum = startingAddress; deviceNum < FRAM_TRUST_SLOTS - FRAM_RESERVED_TRUST_SLOTS; deviceNum++) {
        sprintf((char*)keyBuffer, "%s%03d%c", networkPrefix, deviceNum, TrustActive);
        if (!datastore->recordExists(ZoneTrust, keyBuffer)) {
            memcpy(deviceIdBuffer, keyBuffer, STORAGE_DEVICE_ID_LENGTH);
            deviceIdBuffer[STORAGE_DEVICE_ID_LENGTH] = 0;
            return true;
        }
    }

    // cluster is full
    logConsole("Cluster size maxed out");
    return false;
}

TrustDeviceChannel FramTrustStore::getPreferredChannel (const char* deviceId) {
    populateKeyBuffer(deviceId);
    uint8_t slotNum = datastore->getRecordNum (ZoneTrust, (uint8_t*)keyBuffer);
    if (slotNum >= 0) {
        if (datastore->readRecord(&trustBuffer, slotNum)) {
            return trustBuffer.getPreferredChannel();
        }
        else {
            logConsole("Truststore read failed");
        }
    } 
    else {
        logConsole("Truststore device not found");
    }
    return TrustChannelNone;
}

TrustDeviceChannel FramTrustStore::getSecondaryChannel (const char* deviceId) {
    populateKeyBuffer(deviceId);
    uint8_t slotNum = datastore->getRecordNum (ZoneTrust, (uint8_t*)keyBuffer);
    if (slotNum >= 0) {
        if (datastore->readRecord(&trustBuffer, slotNum)) {
            return trustBuffer.getSecondaryChannel();
        }
        else {
            logConsole("Truststore read failed");
        }
    } 
    else {
        logConsole("Truststore device not found");
    }
    return TrustChannelNone;
}

bool FramTrustStore::removeTrustedDevice (const char* deviceId) {
    populateKeyBuffer(deviceId);
    uint8_t slotNum = datastore->getRecordNum (ZoneTrust, (uint8_t*)keyBuffer);
    if (slotNum >= 0) {
        if (datastore->readRecord(&trustBuffer, slotNum)) {
            trustBuffer.setStatus(TrustDeleted);
            if(datastore->writeRecord(&trustBuffer, slotNum)) {
                logConsole("Trust removed");
                return true;
            }
            else {
                logConsole("Trust removal failed");
                return false;
            }
        }
        else {
            logConsole("Truststore read failed");
            return false;
        }
    } 
    else {
        logConsole("Device already not trusted");
    }
    return true;
}

bool FramTrustStore::clearTruststore () {
  datastore->clearZone(ZoneTrust);
}


bool FramTrustStore::loadPublicKey(const char* deviceId, char* buff) {
    populateKeyBuffer(deviceId);
    uint8_t slotNum = datastore->getRecordNum (ZoneTrust, (uint8_t*)keyBuffer);
    if (slotNum >= 0) {
        if (datastore->readRecord(&trustBuffer, slotNum)) {
            memcpy(buff, trustBuffer.getPublicKey(), STORAGE_PUBLIC_KEY_LENGTH);
            buff[STORAGE_PUBLIC_KEY_LENGTH] = 0;
            return true;
        }
        else {
            logConsole("Truststore read failed");
        }
    } 
    else {
        logConsole("Truststore device not found");
    }
    return false;
}

bool FramTrustStore::loadAlias(const char* deviceId, char* aliasBuffer) {
    populateKeyBuffer(deviceId);
    uint8_t slotNum = datastore->getRecordNum (ZoneTrust, (uint8_t*)keyBuffer);
    if (slotNum >= 0) {
        if (datastore->readRecord(&trustBuffer, slotNum)) {
            memcpy(aliasBuffer, trustBuffer.getAlias(), STORAGE_MAX_ALIAS_LENGTH);
            aliasBuffer[STORAGE_MAX_ALIAS_LENGTH] = 0;
            return true;
        }
        else {
            logConsole("Truststore read failed");
        }
    } 
    else {
        logConsole("Truststore device not found");
    }
    return false;
}

bool FramTrustStore::addTrustedDevice (const char* deviceId, const char* alias, const char* publicKey) {
  return addTrustedDevice (deviceId, alias, publicKey, false);
}

bool FramTrustStore::addTrustedDevice (const char* deviceId, const char* alias, const char* publicKey, bool overwrite) {
    populateKeyBuffer(deviceId);
    uint8_t existingSlot = datastore->getRecordNum(ZoneTrust, (uint8_t*)keyBuffer);
    if (existingSlot >= 0) {
        if (overwrite) {
            if (datastore->readRecord(&trustBuffer, existingSlot)) {
                trustBuffer.setAlias(alias);
                trustBuffer.setPublicKey((uint8_t*)publicKey);
                if (datastore->writeRecord(&trustBuffer, existingSlot)) {
                    logConsole("Trust for device updated");
                    return true;
                }
                else {
                    logConsole("Trust overwrite failed");
                    return false;
                }
            }
            else {
                logConsole("Device read failed");
            }
        }
        else {
            logConsole("Device already trusted. Not overwritten");
            return true;
        }
    }
    else {
        trustBuffer.setDeviceId(deviceId);
        trustBuffer.setAlias(alias);
        trustBuffer.setPublicKey((uint8_t*)publicKey);
        trustBuffer.setPreferredChannel(TrustChannelLora);
        trustBuffer.setSecondaryChannel(TrustChannelUdp);
        if (datastore->writeToNextSlot(&trustBuffer)) {
            logConsole("Trust for device added");
            return true;
        }
        else {
            logConsole("Trust add failed");
            return false;
        }

    }
}
