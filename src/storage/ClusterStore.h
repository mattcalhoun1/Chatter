#include <Arduino.h>
#include <List.hpp>
#include "StorageGlobals.h"
#include "StorageBase.h"

#ifndef CLUSTERSTORE_H
#define CLUSTERSTORE_H

enum ClusterChannel {
  ClusterChannelLora = (uint8_t)'L',
  ClusterChannelUdp = (uint8_t)'U',
  ClusterChannelNone = (uint8_t)'0',
};

enum ClusterStatus {
    ClusterActive = (uint8_t)'A',
    ClusterDeleted = (uint8_t)'D',
};

enum ClusterAuthType {
    ClusterAuthFull = (uint8_t)'F', // everything checked
    ClusterAuthNoExpiry = (uint8_t)'X' // everything except expiry checked (if realtime clocks not in use)
};

class ClusterStore : public StorageBase {
    public:
        virtual bool init () = 0;
        virtual List<String> getClusterIds() = 0;
        virtual bool loadDeviceId (const char* clusterId, char* buffer) = 0;
        virtual float getFrequency (const char* clusterId) = 0;
        virtual bool loadAlias (const char* clusterId, char* buffer) = 0;
        virtual bool loadWifiCred (const char* clusterId, char* buffer) = 0;
        virtual bool loadWifiSsid (const char* clusterId, char* buffer) = 0;

        virtual ClusterChannel getPreferredChannel (const char* clusterId) = 0;
        virtual ClusterChannel getSecondaryChannel (const char* clusterId) = 0;
        virtual ClusterAuthType getAuthType (const char* clusterId) = 0;

        virtual bool loadSymmetricKey (const char* clusterId, uint8_t* buffer) = 0;
        virtual bool loadIv (const char* clusterId, uint8_t* buffer) = 0;

        virtual bool deleteCluster (const char* clusterId) = 0;
        virtual bool addCluster (const char* clusterId, const char* alias, const char* deviceId, uint8_t* symmetricKey, uint8_t* iv, float frequency, const char* wifiSsid, const char* wifiCred, ClusterChannel preferredChannel, ClusterChannel secondaryChannel, ClusterAuthType authType) = 0;

};

#endif