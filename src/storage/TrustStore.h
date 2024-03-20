#include <Arduino.h>
#include <List.hpp>
#include "StorageGlobals.h"
#include "StorageBase.h"

#ifndef TRUSTSTORE_H
#define TRUSTSTORE_H

enum TrustDeviceChannel {
  TrustChannelLora = (uint8_t)'L',
  TrustChannelUdp = (uint8_t)'U',
  TrustChannelNone = (uint8_t)'0',
};

class TrustStore : public StorageBase {
    public:
        virtual bool init () = 0;
        virtual List<String> getDeviceIds() = 0;
        virtual uint8_t populateDeviceIndices (const char* clusterId, uint8_t* deviceIndexBuffer) = 0;
        virtual bool loadPublicKey(const char* deviceId, uint8_t* keyBuffer) = 0;
        virtual bool loadDeviceId (uint8_t internalId, char* deviceId) = 0;
        virtual bool loadAlias(const char* deviceId, char* aliasBuffer) = 0;
        virtual bool addTrustedDevice (const char* deviceId, const char* alias, const uint8_t* publicKey) = 0;
        virtual bool addTrustedDevice (const char* deviceId, const char* alias, const uint8_t* key, bool overwrite) = 0;
        virtual bool removeTrustedDevice (const char* deviceId) = 0;
        virtual bool clearTruststore () = 0;
        virtual bool findDeviceId (const uint8_t* key, const char* clusterId, char* deviceIdBuffer) = 0;
        virtual bool isDeviceTrusted (const char* clusterId, char* deviceId) = 0;
        virtual bool findNextAvailableDeviceId (const char* networkPrefix, int startingAddress, char* deviceIdBuffer) = 0;
        virtual TrustDeviceChannel getPreferredChannel (const char* deviceId) {return TrustChannelLora;}
        virtual TrustDeviceChannel getSecondaryChannel (const char* deviceId) {return TrustChannelUdp;}
};

#endif