#include <Arduino.h>
#include "StorageGlobals.h"
#include "StorageBase.h"

#ifndef DEVICESTORE_H
#define DEVICESTORE_H

class DeviceStore : public StorageBase {
    public:
        virtual bool init () = 0;

        virtual bool loadDeviceName (char* buffer) = 0;
        virtual bool getDefaultClusterId (char* buffer) = 0;
        virtual bool loadSigningKey (uint8_t* buffer) = 0;

        virtual bool setDeviceName (const char* newDeviceName) = 0;
        virtual bool setDefaultClusterId (const char* newDefaultCluster) = 0;
        virtual bool setSigningKey (const uint8_t* newSigningKey) = 0;
};

#endif