#include <Arduino.h>
#include "../StorageGlobals.h"
#include "FramData.h"
#include "FramGlobals.h"
#include "../DeviceStore.h"
#include "DeviceConfig.h"

#ifndef FRAMDEVICESTORE_H
#define FRAMDEVICESTORE_H

#define FRAM_DEV_KEY_NAME "CHATTERDEVCCNAME"
#define FRAM_DEV_KEY_DEFAULT_CLUSTER "DEFAULTCLUSTERID"
#define FRAM_DEV_KEY_SIGN_KEY "CHATTERSINGPVKEY"

class FramDeviceStore : public DeviceStore {
    public:
        FramDeviceStore(FramData* _datastore) { datastore = _datastore; }

        bool init ();

        bool loadDeviceName (char* buffer);
        bool getDefaultClusterId (char* buffer);
        bool loadSigningKey (uint8_t* buffer);

        bool setDeviceName (const char* newName);
        bool setDefaultClusterId (const char* newDefaultCluster);
        bool setSigningKey (const uint8_t* newSigningKey);

    protected:
        FramData* datastore;
        DeviceConfig configBuffer;

        bool loadBuffer (const char* deviceKeyName, char* buffer, int maxLength);
        bool saveConfig (const char* deviceKeyName, const char* newVal);
};

#endif