#include "FramDeviceStore.h"

bool FramDeviceStore::init () {
    return true;
}

bool FramDeviceStore::loadBuffer (const char* deviceKeyName, char* buffer, int maxLength) {
    memset (buffer, 0, maxLength);
    uint8_t slotNum = datastore->getRecordNum(ZoneDevice, (uint8_t*)deviceKeyName);

    if (slotNum != FRAM_NULL && datastore->readRecord(&configBuffer, slotNum)) {
        memcpy(buffer, configBuffer.getValue(), maxLength);
        return true;
    }

    return false;
}


bool FramDeviceStore::loadDeviceName (char* buffer) {
    return loadBuffer(FRAM_DEV_KEY_NAME, buffer, CHATTER_ALIAS_NAME_SIZE);
}

bool FramDeviceStore::getDefaultClusterId (char* buffer) {
    return loadBuffer(FRAM_DEV_KEY_DEFAULT_CLUSTER, buffer, CHATTER_LOCAL_NET_ID_SIZE + CHATTER_GLOBAL_NET_ID_SIZE);
}

bool FramDeviceStore::loadSigningKey (uint8_t* buffer) {
    return loadBuffer(FRAM_DEV_KEY_SIGN_KEY, (char*)buffer, ENC_PRIV_KEY_SIZE);
}

bool FramDeviceStore::setDeviceName (const char* newName) {
    bool success = saveConfig(FRAM_DEV_KEY_NAME, newName);
    if (success) {
        logConsole("device name saved");
    }
    else {
        logConsole("device name not saved!");
    }
    return success;
}

bool FramDeviceStore::setDefaultClusterId (const char* newDefaultCluster) {
    bool success = saveConfig(FRAM_DEV_KEY_DEFAULT_CLUSTER, newDefaultCluster);
    if (success) {
        logConsole("default cluster saved");
    }
    else {
        logConsole("default cluster not saved!");
    }
    return success;
}

bool FramDeviceStore::setSigningKey (const uint8_t* newSigningKey) {
    bool success = saveConfig(FRAM_DEV_KEY_SIGN_KEY, (char*)newSigningKey);
    if (success) {
        logConsole("signing key saved");
    }
    else {
        logConsole("signing key not saved!");
    }
    return success;
    
}

bool FramDeviceStore::saveConfig (const char* deviceKeyName, const char* newVal) {
    configBuffer.setKey(deviceKeyName);
    configBuffer.setValue((uint8_t*)newVal);

    Serial.print("Saving config -> "); Serial.print(deviceKeyName); Serial.print(":"); Serial.println(newVal);

    uint8_t slot = datastore->getRecordNum(ZoneDevice, (uint8_t*)deviceKeyName);
    if (slot == FRAM_NULL) {
        return datastore->writeToNextSlot(&configBuffer);
    }
    return datastore->writeRecord(&configBuffer, slot);
}

