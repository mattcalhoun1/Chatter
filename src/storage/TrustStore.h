#include <Arduino.h>
#include <List.hpp>
#include "StorageGlobals.h"
#include "StorageBase.h"

#ifndef TRUSTSTORE_H
#define TRUSTSTORE_H

class TrustStore : public StorageBase {
    public:
        virtual bool init () = 0;
        virtual List<String> getDeviceIds() = 0;
        virtual bool loadPublicKey(const char* deviceId, char* keyBuffer) = 0;
        virtual bool loadAlias(const char* deviceId, char* aliasBuffer) = 0;
        virtual bool addTrustedDevice (const char* deviceId, const char* alias, const char* publicKey) = 0;
        virtual bool addTrustedDevice (const char* deviceId, const char* alias, const char* key, bool overwrite) = 0;
        virtual bool removeTrustedDevice (const char* deviceId) = 0;
        virtual bool clearTruststore () = 0;
        virtual bool findDeviceId (const char* key, char* deviceIdBuffer) = 0;
};

#endif