#include "ClusterLicense.h"

void ClusterLicense::setClusterId (const char* _clusterId) {
  memcpy(clusterId, _clusterId, STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE);
}

void ClusterLicense::setSignerId (const char* _signerId) {
  memcpy(signerId, _signerId, CHATTER_DEVICE_ID_SIZE);
}

void ClusterLicense::setLicense(const uint8_t* _license) {
  memcpy(license, _license, ENC_SIG_BUFFER_SIZE);
}

void ClusterLicense::deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer) {
    // record key is the device id
    memcpy(clusterId, recordKey, STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE);
    status = (LicenseStatus)recordKey[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE];

    // data buffer is in the following order
    const uint8_t* bufferPos = dataBuffer;
    memcpy(signerId, bufferPos, CHATTER_DEVICE_ID_SIZE);
    bufferPos += CHATTER_DEVICE_ID_SIZE;

    memcpy(license, bufferPos, ENC_SIG_BUFFER_SIZE);
}

int ClusterLicense::serialize (uint8_t* dataBuffer) {
    // data buffer is in the following order (total of 74 bytes)
    uint8_t* bufferPos = dataBuffer;

    memcpy(bufferPos, signerId, CHATTER_DEVICE_ID_SIZE);
    bufferPos += CHATTER_DEVICE_ID_SIZE;

    memcpy(bufferPos, license, ENC_SIG_BUFFER_SIZE);
    //bufferPos += CHATTER_LORA_FREQUENCY_DIGITS;

    // length is always the same
    return FRAM_LICENSE_DATASIZE;
}

void ClusterLicense::serializeKey (uint8_t* keyBuffer) {
  memcpy(keyBuffer, clusterId, STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE);
  keyBuffer[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE] = status;
}
