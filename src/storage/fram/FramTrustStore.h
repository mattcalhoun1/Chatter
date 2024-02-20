#include <Arduino.h>
#include <List.hpp>
#include "../StorageGlobals.h"
#include "../StorageBase.h"
#include "../StorageGlobals.h"
#include "../PacketStore.h"
#include "FramData.h"
#include "FramGlobals.h"
#include "TrustConfig.h"
#include "../TrustStore.h"

#ifndef FRAMTRUSTSTORE_H
#define FRAMTRUSTSTORE_H

class FramTrustStore : public TrustStore {
    public:
        FramTrustStore(FramData* _datastore) { datastore = _datastore; }

        bool init ();
        List<String> getDeviceIds();
        bool loadPublicKey(const char* deviceId, char* keyBuffer);
        bool loadAlias(const char* deviceId, char* aliasBuffer);
        bool addTrustedDevice (const char* deviceId, const char* alias, const char* publicKey);
        bool addTrustedDevice (const char* deviceId, const char* alias, const char* key, bool overwrite);
        bool removeTrustedDevice (const char* deviceId);
        bool clearTruststore ();
        bool findDeviceId (const char* key, char* deviceIdBuffer);
        bool findNextAvailableDeviceId (const char* networkPrefix, int startingAddress, char* deviceIdBuffer);
        TrustDeviceChannel getPreferredChannel (const char* deviceId);
        TrustDeviceChannel getSecondaryChannel (const char* deviceId);

    private:
        void populateKeyBuffer (const char* deviceId);
        FramData* datastore;
        uint8_t keyBuffer[FRAM_TRUST_KEYSIZE + 1];
        TrustConfig trustBuffer;

};

#endif