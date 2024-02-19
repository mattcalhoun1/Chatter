#include <Arduino.h>
#include "FramData.h"
#include "Globals.h"

#ifndef CLUSTERCONFIG_H
#define CLUSTERCONFIG_H

class ClusterConfig : public FramRecord {
  public:
    FramZone getZone () { return ZoneCluster; }

    void setDeviceId (const char* _deviceId);
    const char* getDeviceId () {return (const char*)deviceId;}

    void setFrequency (const char* _frequency);
    const char* getFrequency () {return (const char*)frequency;}

    void setKey(const uint8_t* _key);
    const uint8_t* getKey () {return key;}

    void setIv(const uint8_t* _iv);
    const uint8_t* getIv () {return iv;}

    void setAlias (const char* _alias);
    const char* getAlias () {return (const char*)alias;}

    void setWifi (const char* _wifi);
    const char* getWifi () {return (const char*)wifi;}


    void deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer);
    void serialize (uint8_t* dataBuffer);
    void serializeKey (uint8_t* keyBuffer);

  protected:
    // unencrypted
    char deviceId[CHATTER_DEVICE_ID_SIZE];

    // total unecrypted bytes (not including key above)= 74
    // encrypted
    // frequency
    // key
    // iv
    // device alias
    // wifi

    char frequency[CHATTER_LORA_FREQUENCY_DIGITS];
    uint8_t key[ENC_SYMMETRIC_KEY_SIZE];
    uint8_t iv[ENC_IV_SIZE];
    char alias[CHATTER_ALIAS_NAME_SIZE];
    char wifi[CHATTER_WIFI_STRING_MAX_SIZE];
};

#endif