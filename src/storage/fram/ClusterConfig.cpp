#include "ClusterConfig.h"

void ClusterConfig::setClusterId (const char* _clusterId) {
  memcpy(clusterId, _clusterId, STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE);
}

void ClusterConfig::setDeviceId (const char* _deviceId) {
  memcpy(deviceId, _deviceId, CHATTER_DEVICE_ID_SIZE);
}

void ClusterConfig::setFrequency (const char* _frequency) {
    Serial.print("Setting freq: ");
    for (int i = 0; i < 5; i++) {
        Serial.print(_frequency[i]);
    }
    Serial.println("");

  memset(frequency, 0, CHATTER_LORA_FREQUENCY_DIGITS);
  memcpy(frequency, _frequency, min(strlen(_frequency), CHATTER_LORA_FREQUENCY_DIGITS));
}

void ClusterConfig::setKey(const uint8_t* _key) {
  memcpy(key, _key, ENC_SYMMETRIC_KEY_SIZE);
}

void ClusterConfig::setIv(const uint8_t* _iv) {
  memcpy(iv, _iv, ENC_IV_SIZE);
}

void ClusterConfig::setAlias (const char* _alias) {
  memset(alias, 0, CHATTER_ALIAS_NAME_SIZE);
  memcpy(alias, _alias, min(strlen(_alias), CHATTER_ALIAS_NAME_SIZE));
}

void ClusterConfig::setWifiSsid (const char* _wifiSsid) {
  memset(wifiSsid, 0, CHATTER_WIFI_STRING_MAX_SIZE);
  memcpy(wifiSsid, _wifiSsid, min(strlen(_wifiSsid), CHATTER_WIFI_STRING_MAX_SIZE));
}

void ClusterConfig::setWifiCred (const char* _wifiCred) {
  memset(wifiCred, 0, CHATTER_WIFI_STRING_MAX_SIZE);
  memcpy(wifiCred, _wifiCred, min(strlen(_wifiCred), CHATTER_WIFI_STRING_MAX_SIZE));
}

void ClusterConfig::deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer) {
    // record key is the device id
    memcpy(clusterId, recordKey, STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE);
    status = (ClusterStatus)recordKey[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE];

    // data buffer is in the following order
    const uint8_t* bufferPos = dataBuffer;
    preferredChannel = (ClusterChannel)bufferPos[0];
    secondaryChannel = (ClusterChannel)bufferPos[1];
    authType = (ClusterAuthType)bufferPos[2];
    bufferPos += 3;

    memcpy(deviceId, bufferPos, CHATTER_DEVICE_ID_SIZE);
    bufferPos += CHATTER_DEVICE_ID_SIZE;

    memcpy(frequency, bufferPos, CHATTER_LORA_FREQUENCY_DIGITS);
    bufferPos += CHATTER_LORA_FREQUENCY_DIGITS;

    memcpy(key, bufferPos, ENC_SYMMETRIC_KEY_SIZE);
    bufferPos += ENC_SYMMETRIC_KEY_SIZE;

    memcpy(iv, bufferPos, ENC_IV_SIZE);
    bufferPos += ENC_IV_SIZE;

    memcpy(alias, bufferPos, CHATTER_ALIAS_NAME_SIZE);
    bufferPos += CHATTER_ALIAS_NAME_SIZE;

    memcpy(wifiSsid, bufferPos, CHATTER_WIFI_STRING_MAX_SIZE);
    bufferPos += CHATTER_WIFI_STRING_MAX_SIZE;

    memcpy(wifiCred, bufferPos, CHATTER_WIFI_STRING_MAX_SIZE);
}

int ClusterConfig::serialize (uint8_t* dataBuffer) {
    // data buffer is in the following order (total of 74 bytes)
    uint8_t* bufferPos = dataBuffer;
    bufferPos[0] = preferredChannel;
    bufferPos[1] = secondaryChannel;
    bufferPos[2] = authType;
    bufferPos += 3;

    memcpy(bufferPos, deviceId, CHATTER_DEVICE_ID_SIZE);
    bufferPos += CHATTER_DEVICE_ID_SIZE;

    memcpy(bufferPos, frequency, CHATTER_LORA_FREQUENCY_DIGITS);
    bufferPos += CHATTER_LORA_FREQUENCY_DIGITS;

    memcpy(bufferPos, key, ENC_SYMMETRIC_KEY_SIZE);
    bufferPos += ENC_SYMMETRIC_KEY_SIZE;

    memcpy(bufferPos, iv, ENC_IV_SIZE);
    bufferPos += ENC_IV_SIZE;

    memcpy(bufferPos, alias, CHATTER_ALIAS_NAME_SIZE);
    bufferPos += CHATTER_ALIAS_NAME_SIZE;

    memcpy(bufferPos, wifiSsid, CHATTER_WIFI_STRING_MAX_SIZE);
    bufferPos += CHATTER_WIFI_STRING_MAX_SIZE;

    memcpy(bufferPos, wifiCred, CHATTER_WIFI_STRING_MAX_SIZE);

    // length is always the same
    return FRAM_CLUSTER_DATASIZE;
}

void ClusterConfig::serializeKey (uint8_t* keyBuffer) {
  memcpy(keyBuffer, clusterId, STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE);
  keyBuffer[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE] = status;
}
