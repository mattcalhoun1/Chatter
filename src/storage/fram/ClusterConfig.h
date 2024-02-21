#include <Arduino.h>
#include "FramData.h"
#include "FramGlobals.h"
#include "../ClusterStore.h"

#ifndef CLUSTERCONFIG_H
#define CLUSTERCONFIG_H

class ClusterConfig : public FramRecord {
  public:
    FramZone getZone () { return ZoneCluster; }

    void setClusterId (const char* _clusterId);
    const char* getClusterId () {return (const char*)clusterId;}

    void setStatus (ClusterStatus _status) {status = _status;}
    ClusterStatus getStatus () {return status;}

    void setPreferredChannel (ClusterChannel _preferred) {preferredChannel = _preferred;}
    ClusterChannel getPreferredChannel () {return preferredChannel;}

    void setSecondaryChannel (ClusterChannel _secondary) {secondaryChannel = _secondary;}
    ClusterChannel getSecondaryChannel () {return secondaryChannel;}

    void setAuthType (ClusterAuthType _authType) {authType = _authType;}
    ClusterAuthType getAuthType () {return authType;}

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

    void setWifiSsid (const char* _wifiSsid);
    const char* getWifiSsid () {return (const char*)wifiSsid;}

    void setWifiCred (const char* _wifiCred);
    const char* getWifiCred () {return (const char*)wifiCred;}


    void deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer);
    int serialize (uint8_t* dataBuffer);
    void serializeKey (uint8_t* keyBuffer);

  protected:
    char clusterId[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE];
    ClusterStatus status;

    ClusterChannel preferredChannel;
    ClusterChannel secondaryChannel;
    ClusterAuthType authType;

    char deviceId[CHATTER_DEVICE_ID_SIZE];
    char frequency[CHATTER_LORA_FREQUENCY_DIGITS];
    uint8_t key[ENC_SYMMETRIC_KEY_SIZE];
    uint8_t iv[ENC_IV_SIZE];
    char alias[CHATTER_ALIAS_NAME_SIZE];
    char wifiSsid[CHATTER_WIFI_STRING_MAX_SIZE];
    char wifiCred[CHATTER_WIFI_STRING_MAX_SIZE];
};

#endif