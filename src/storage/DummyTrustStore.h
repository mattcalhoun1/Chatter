#include "TrustStore.h"

#ifndef DUMMYTRUSTSTORE_H
#define DUMMYTRUSTSTORE_H

class DummyTrustStore : public TrustStore {
    public:
        bool init ();
        List<String> getDeviceIds();
        bool loadPublicKey(const char* deviceId, char* keyBuffer);
        bool loadAlias(const char* deviceId, char* aliasBuffer);
        bool addTrustedDevice (const char* deviceId, const char* alias, const char* publicKey);
        bool addTrustedDevice (const char* deviceId, const char* alias, const char* key, bool overwrite);
        bool removeTrustedDevice (const char* deviceId) {return true; }
        bool clearTruststore () { return true; }
        bool findDeviceId (const char* key, char* deviceIdBuffer) {return false;}
        bool findNextAvailableDeviceId (const char* networkPrefix, int startingAddress, char* deviceIdBuffer) {return false;}
};

#endif