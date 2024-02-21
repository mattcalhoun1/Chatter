#include "FramDeviceStore.h"

bool FramDeviceStore::init () {
    return true;
}

bool FramDeviceStore::loadBuffer (const char* deviceKeyName, char* buffer) {
    uint8_t slotNum = datastore->getRecordNum(ZoneDevice, (uint8_t*)deviceKeyName);
    if (slotNum != FRAM_NULL && datastore->readRecord(&configBuffer, slotNum)) {
        memcpy(buffer, configBuffer.getValue(), FRAM_DEVICE_DATASIZE_USABLE);
        return true;
    }

    return false;
}


bool FramDeviceStore::loadDeviceName (char* buffer) {
    return loadBuffer(FRAM_DEV_KEY_NAME, buffer);
}

bool FramDeviceStore::getDefaultClusterId (char* buffer) {
    return loadBuffer(FRAM_DEV_KEY_DEFAULT_CLUSTER, buffer);
}

bool FramDeviceStore::loadSigningKey (uint8_t* buffer) {
    return loadBuffer(FRAM_DEV_KEY_SIGN_KEY, (char*)buffer);
}

bool FramDeviceStore::setDeviceName (char* newName) {
    bool success = saveConfig(FRAM_DEV_KEY_NAME, newName);
    if (success) {
        logConsole("device name saved");
    }
    else {
        logConsole("device name not saved!");
    }
    return success;
}

bool FramDeviceStore::setDefaultClusterId (char* newDefaultCluster) {
    bool success = saveConfig(FRAM_DEV_KEY_DEFAULT_CLUSTER, newDefaultCluster);
    if (success) {
        logConsole("default cluster saved");
    }
    else {
        logConsole("default cluster not saved!");
    }
    return success;
}

bool FramDeviceStore::setSigningKey (uint8_t* newSigningKey) {
    bool success = saveConfig(FRAM_DEV_KEY_SIGN_KEY, (char*)newSigningKey);
    if (success) {
        logConsole("signing key saved");
    }
    else {
        logConsole("signing key not saved!");
    }
    return success;
    
}

bool FramDeviceStore::saveConfig (const char* deviceKeyName, char* newVal) {
    configBuffer.setKey(deviceKeyName);
    configBuffer.setValue((uint8_t*)newVal);

    uint8_t slot = datastore->getRecordNum(ZoneDevice, (uint8_t*)deviceKeyName);
    if (slot == FRAM_NULL) {
        return datastore->writeToNextSlot(&configBuffer);
    }
    return datastore->writeRecord(&configBuffer, slot);
}

