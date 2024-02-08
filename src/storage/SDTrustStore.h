#include <Arduino.h>
#include <List.hpp>
#include "StorageGlobals.h"
#include <SD.h>
#include "StorageBase.h"
#include "TrustStore.h"

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
    private:
        bool writeLineToFile (File* file, const char* content);
        const char* publicKeysDir = "/trusted/";
};

#endif