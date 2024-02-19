#include "TrustConfig.h"

void TrustConfig::setDeviceId (const char* _deviceId) {
  memcpy(deviceId, _deviceId, CHATTER_DEVICE_ID_SIZE);
}

void TrustConfig::setPublicKey(const uint8_t* _publicKey) {
  memcpy(publicKey, _publicKey, ENC_PUB_KEY_SIZE);
}

void TrustConfig::setAlias (const char* _alias) {
  memset(alias, 0, CHATTER_ALIAS_NAME_SIZE);
  memcpy(alias, _alias, min(strlen(_alias), CHATTER_ALIAS_NAME_SIZE));
}

void TrustConfig::deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer) {
  // record key is the device id

  memcpy(deviceId, recordKey, CHATTER_DEVICE_ID_SIZE);

  const uint8_t* bufferPos = dataBuffer;
  memcpy(publicKey, bufferPos, ENC_PUB_KEY_SIZE);
  bufferPos += ENC_PUB_KEY_SIZE;

  memcpy(alias, bufferPos, CHATTER_ALIAS_NAME_SIZE);
}

void TrustConfig::serialize (uint8_t* dataBuffer) {
  uint8_t* bufferPos = dataBuffer;
  memcpy(bufferPos, publicKey, ENC_PUB_KEY_SIZE);
  bufferPos += ENC_PUB_KEY_SIZE;

  memcpy(bufferPos, alias, CHATTER_ALIAS_NAME_SIZE);
}

void TrustConfig::serializeKey (uint8_t* keyBuffer) {
  memcpy(keyBuffer, deviceId, CHATTER_DEVICE_ID_SIZE);
}
