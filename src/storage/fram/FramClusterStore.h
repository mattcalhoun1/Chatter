#include <Arduino.h>
#include "../StorageGlobals.h"
#include "../ClusterStore.h"
#include "FramData.h"
#include "FramGlobals.h"
#include "ClusterConfig.h"
#include <List.hpp>

#ifndef FRAMCLUSTERSTORE_H
#define FRAMCLUSTERSTORE_H

class FramClusterStore : public ClusterStore {
    public:
        FramClusterStore(FramData* _datastore) { datastore = _datastore; }
        bool init ();
        List<String> getClusterIds();
        bool loadDeviceId (const char* clusterId, char* buffer);
        float getFrequency (const char* clusterId);
        bool loadAlias (const char* clusterId, char* buffer);
        bool loadWifiCred (const char* clusterId, char* buffer);
        bool loadWifiSsid (const char* clusterId, char* buffer);

        ClusterChannel getPreferredChannel (const char* clusterId);
        ClusterChannel getSecondaryChannel (const char* clusterId);
        ClusterAuthType getAuthType (const char* clusterId);

        bool loadSymmetricKey (const char* clusterId, uint8_t* buffer);
        bool loadIv (const char* clusterId, uint8_t* buffer);

        bool deleteCluster (const char* clusterId);
        bool addCluster (const char* clusterId,const char* alias, uint8_t* symmetricKey, uint8_t* iv, float frequency, const char* wifiSsid, const char* wifiCred, ClusterChannel preferredChannel, ClusterChannel secondaryChannel, ClusterAuthType authType);

    protected:
        FramData* datastore;

        bool loadClusterBuffer (const char* clusterId);
        ClusterConfig clusterBuffer;
        bool clusterLoaded = false; // just indicates whether cluster buffer has been populated with ANYthing
        char keyBuffer[FRAM_CLUSTER_KEYSIZE];
        void populateKeyBuffer (const char* clusterId);

};
#endif