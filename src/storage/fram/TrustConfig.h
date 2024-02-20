// all unencrypted
// 128 pub key
// device id
// device alias

#include <Arduino.h>
#include "FramData.h"
#include "FramGlobals.h"

#ifndef TRUSTCONFIG_H
#define TRUSTCONFIG_H

class TrustConfig : public FramRecord {
  public:
    FramZone getZone () { return ZoneTrust; }

    void setDeviceId (const char* _deviceId);
    const char* getDeviceId () {return (const char*)deviceId;}

    void setPublicKey(const uint8_t* _publicKey);
    const uint8_t* getPublicKey () {return publicKey;}

    void setAlias (const char* _alias);
    const char* getAlias () {return (const char*)alias;}


    void deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer);
    int serialize (uint8_t* dataBuffer);
    void serializeKey (uint8_t* keyBuffer);

  protected:
    char deviceId[CHATTER_DEVICE_ID_SIZE];
    uint8_t publicKey[ENC_PUB_KEY_SIZE];
    char alias[CHATTER_ALIAS_NAME_SIZE];
};

#endif
