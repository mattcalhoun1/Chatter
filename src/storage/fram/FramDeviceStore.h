#include <Arduino.h>
#include "../StorageGlobals.h"
#include "FramData.h"
#include "FramGlobals.h"
#include "../DeviceStore.h"
#include "DeviceConfig.h"

#ifndef FRAMDEVICESTORE_H
#define FRAMDEVICESTORE_H

#define FRAM_DEV_KEY_NAME "CHATTERDEVCNAME"
#define FRAM_DEV_KEY_DEFAULT_CLUSTER "CDEFAULTCLUSTER"
#define FRAM_DEV_KEY_SIGN_KEY "CHATTSIGNINGKEY"

class FramDeviceStore : public DeviceStore {
    public:
        FramDeviceStore(FramData* _datastore) { datastore = _datastore; }

        bool init ();

        bool loadDeviceName (char* buffer);
        bool getDefaultClusterId (char* buffer);
        bool loadSigningKey (uint8_t* buffer);

        bool setDeviceName (char* newName);
        bool setDefaultCluster (char* newDefaultCluster);
        bool setSigningKey (uint8_t* newSigningKey);

    protected:
        FramData* datastore;
        DeviceConfig configBuffer;

        bool loadBuffer (const char* deviceKeyName, char* buffer);
        bool saveConfig (const char* deviceKeyName, char* newVal);
};

#endif