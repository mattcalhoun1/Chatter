// all unencrypted
// 128 pub key
// device id
// device alias

#include <Arduino.h>
#include "FramData.h"
#include "FramGlobals.h"
#include "../TrustStore.h"

#ifndef TRUSTCONFIG_H
#define TRUSTCONFIG_H

enum TrustConfigStatus {
  TrustActive = (uint8_t)'A',
  TrustDeleted = (uint8_t)'D'
};

class TrustConfig : public FramRecord {
  public:
    FramZone getZone () { return ZoneTrust; }

    void setDeviceId (const char* _deviceId);
    const char* getDeviceId () {return (const char*)deviceId;}

    void setStatus (TrustConfigStatus _status) {status = _status;}
    TrustConfigStatus getStatus () {return status;}

    void setPreferredChannel (TrustDeviceChannel _preferred) {preferredChannel = _preferred;}
    TrustDeviceChannel getPreferredChannel () {return preferredChannel;}

    void setSecondaryChannel (TrustDeviceChannel _secondary) {secondaryChannel = _secondary;}
    TrustDeviceChannel getSecondaryChannel () {return secondaryChannel;}

    void setPublicKey(const uint8_t* _publicKey);
    const uint8_t* getPublicKey () {return publicKey;}

    void setAlias (const char* _alias);
    const char* getAlias () {return (const char*)alias;}


    void deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer);
    int serialize (uint8_t* dataBuffer);
    void serializeKey (uint8_t* keyBuffer);

  protected:
    char deviceId[CHATTER_DEVICE_ID_SIZE];
    TrustConfigStatus status = TrustActive;
    TrustDeviceChannel preferredChannel = TrustChannelLora;
    TrustDeviceChannel secondaryChannel = TrustChannelUdp;

    uint8_t publicKey[ENC_PUB_KEY_SIZE];
    char alias[CHATTER_ALIAS_NAME_SIZE];
};

#endif
