#include "DeviceConfig.h"

void DeviceConfig::setKey (const char* _key) {
    memset(key, 0, FRAM_DEVICE_KEYSIZE);
    memcpy(key, _key, min(strlen(_key), FRAM_DEVICE_KEYSIZE));
}

void DeviceConfig::setValue (const uint8_t* _val) {
    memset(val, 0, FRAM_DEVICE_DATASIZE);
    memcpy(val, _val, FRAM_DEVICE_DATASIZE_USABLE);
}

void DeviceConfig::deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer) {
  // record key is simply the key
  memcpy(key, recordKey, FRAM_DEVICE_KEYSIZE);

  // val is the entire data buffer
  memset(val, 0, FRAM_DEVICE_DATASIZE);
  memcpy(val, dataBuffer, FRAM_DEVICE_DATASIZE_USABLE);
}

int DeviceConfig::serialize (uint8_t* dataBuffer) {
  // val is the entire data buffer
  memset(dataBuffer, 0, FRAM_DEVICE_DATASIZE);
  memcpy(dataBuffer, val, FRAM_DEVICE_DATASIZE_USABLE);

  // length is always the same
  return FRAM_DEVICE_DATASIZE;
}

void DeviceConfig::serializeKey (uint8_t* keyBuffer) {
  memcpy(keyBuffer, key, FRAM_DEVICE_KEYSIZE);

}
