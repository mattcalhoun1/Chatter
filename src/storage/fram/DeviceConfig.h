#include <Arduino.h>
#include "FramData.h"
#include "FramGlobals.h"

#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H

class DeviceConfig : public FramRecord {
  public:
    FramZone getZone () { return ZoneDevice; }

    void setKey (const char* _key);
    const char* getKey () {return (const char*)key;}

    void setValue (const uint8_t* _val);
    const uint8_t* getValue () {return val;}

    void deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer);
    int serialize (uint8_t* dataBuffer);
    void serializeKey (uint8_t* keyBuffer);

  protected:
    // unencrypted
    char key[FRAM_DEVICE_KEYSIZE];

    // encrypted
    uint8_t val[FRAM_DEVICE_DATASIZE];
};

#endif