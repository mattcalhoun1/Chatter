#include <Arduino.h>
#include <List.hpp>
#include "../StorageGlobals.h"
#include <SD.h>
#include "../StorageBase.h"
#include "../TrustStore.h"

#ifndef SDTRUSTSTORE_H
#define SDTRUSTSTORE_H

class SDTrustStore : public TrustStore {
    public:
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

    private:
        bool writeLineToFile (File* file, const char* content, int maxBytes);
        const char* publicKeysDir = "/trst/";

        char fullFileName[STORAGE_MAX_TRUSTSTORE_FILENAME_LENGTH];
        void populateFullFileName(const char* deviceId);
};

#endif