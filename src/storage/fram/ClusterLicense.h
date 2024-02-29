#include <Arduino.h>
#include "FramData.h"
#include "FramGlobals.h"
#include "../LicenseStore.h"

#ifndef CLUSTERLICENSE_H
#define CLUSTERLICENSE_H

class ClusterLicense : public FramRecord {
  public:
    FramZone getZone () { return ZoneLicense; }

    void setClusterId (const char* _clusterId);
    const char* getClusterId () {return (const char*)clusterId;}

    void setStatus (LicenseStatus _status) {status = _status;}
    LicenseStatus getStatus () {return status;}

    void setSignerId (const char* _signerId);
    const char* getSignerId () {return (const char*)signerId;}

    void setLicense (const uint8_t* _license);
    const uint8_t* getLicense () {return (const uint8_t*)license;}

    void deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer);
    int serialize (uint8_t* dataBuffer);
    void serializeKey (uint8_t* keyBuffer);

  protected:
    char clusterId[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE];
    LicenseStatus status;

    char signerId[CHATTER_DEVICE_ID_SIZE];
    uint8_t license[ENC_SIG_BUFFER_SIZE];
};

#endif