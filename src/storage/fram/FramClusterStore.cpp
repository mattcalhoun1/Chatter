#include "FramClusterStore.h"

bool FramClusterStore::init () {
    return true;
}

bool FramClusterStore::loadClusterBuffer (const char* clusterId) {
    // if this is already loaded, dont do anything
    if (clusterLoaded == false || memcmp(clusterId, clusterBuffer.getClusterId(), STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE) != 0) {
        populateKeyBuffer(clusterId);
        uint8_t slotNum = datastore->getRecordNum(ZoneCluster, (uint8_t*)keyBuffer);
        if (slotNum >= 0) {
            return datastore->readRecord(&clusterBuffer, slotNum);
        }
        else {
            logConsole("Cluster not found by key");
            return false;
        }
    }

    // cluster already loaded into buffer
    return true;
}

void FramClusterStore::populateKeyBuffer (const char* clusterId) {
    memcpy(keyBuffer, clusterId, FRAM_CLUSTER_KEYSIZE - 1);
    keyBuffer[FRAM_CLUSTER_KEYSIZE - 1] = ClusterActive;
}

List<String> FramClusterStore::getClusterIds() {
    List<String> clusterIds;

    uint8_t slotsUsed = datastore->getNumUsedSlots(ZoneCluster);
    for (uint8_t slot = 0; slot < slotsUsed; slot++) {
        datastore->readKey((uint8_t*)keyBuffer, ZoneCluster, slot);
        if (keyBuffer[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE] == ClusterActive) {
            // trim the status for these purposes
            keyBuffer[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE] = 0;
            clusterIds.add(String(keyBuffer));
        }
    }

    return clusterIds;
}

bool FramClusterStore::loadDeviceId (const char* clusterId, char* buffer) {
    if (loadClusterBuffer(clusterId)) {
        memcpy(buffer, clusterBuffer.getDeviceId(), STORAGE_DEVICE_ID_LENGTH);
        buffer[STORAGE_CHUNK_ID_LENGTH] = 0;
        return true;
    }
    return false;
}

float FramClusterStore::getFrequency (const char* clusterId) {
    if (loadClusterBuffer(clusterId)) {
        return atof(clusterBuffer.getFrequency());
    }
    return -1;
}

bool FramClusterStore::loadAlias (const char* clusterId, char* buffer) {
    if (loadClusterBuffer(clusterId)) {
        uint8_t bytes = min(strlen(clusterBuffer.getAlias()),STORAGE_MAX_ALIAS_LENGTH);
        memcpy(buffer, clusterBuffer.getAlias(), bytes);
        buffer[bytes] = 0;
        return true;
    }
    return false;
}

bool FramClusterStore::loadWifiCred (const char* clusterId, char* buffer) {
    if (loadClusterBuffer(clusterId)) {
        uint8_t bytes = min(strlen(clusterBuffer.getWifiCred()), CHATTER_WIFI_STRING_MAX_SIZE);
        memcpy(buffer, clusterBuffer.getWifiCred(), bytes);
        buffer[bytes] = 0;
        return true;
    }
    return false;
}

bool FramClusterStore::loadWifiSsid (const char* clusterId, char* buffer) {
    if (loadClusterBuffer(clusterId)) {
        uint8_t bytes = min(strlen(clusterBuffer.getWifiSsid()), CHATTER_WIFI_STRING_MAX_SIZE);
        memcpy(buffer, clusterBuffer.getWifiSsid(), bytes);
        buffer[bytes] = 0;
        return true;
    }
    return false;
}

ClusterChannel FramClusterStore::getPreferredChannel (const char* clusterId) {
    if (loadClusterBuffer(clusterId)) {
        return clusterBuffer.getPreferredChannel();
    }
    return ClusterChannelNone;
}

ClusterChannel FramClusterStore::getSecondaryChannel (const char* clusterId) {
    if (loadClusterBuffer(clusterId)) {
        return clusterBuffer.getSecondaryChannel();
    }
    return ClusterChannelNone;
}

bool FramClusterStore::loadSymmetricKey (const char* clusterId, uint8_t* buffer) {
    if (loadClusterBuffer(clusterId)) {
        memcpy(buffer, clusterBuffer.getKey(), ENC_SYMMETRIC_KEY_SIZE);
        return true;
    }
    return false;
}

bool FramClusterStore::loadIv (const char* clusterId, uint8_t* buffer) {
    if (loadClusterBuffer(clusterId)) {
        memcpy(buffer, clusterBuffer.getKey(), ENC_IV_SIZE);
        return true;
    }
    return false;
}

bool FramClusterStore::deleteCluster (const char* clusterId) {
    if (loadClusterBuffer(clusterId)) {
        clusterBuffer.setStatus(ClusterDeleted);

        populateKeyBuffer(clusterId);

        uint8_t slotNum = datastore->getRecordNum(ZoneCluster, (uint8_t*)clusterId);
        if (slotNum >= 0) {
            bool result = datastore->writeRecord(&clusterBuffer, slotNum);
            if (result) {
                logConsole("Deleted cluster");
            }
            else {
                logConsole("Failed to delete cluster");
            }
            return result;
        }
        return false;
    }
    return true; // if not found, its already deleted
}

bool FramClusterStore::addCluster (const char* clusterId, const char* alias, uint8_t* symmetricKey, uint8_t* iv, float frequency, const char* wifiSsid, const char* wifiCred, ClusterChannel preferredChannel, ClusterChannel secondaryChannel) {
    // make sure cluster doesnt already exist
    populateKeyBuffer(clusterId);

    if (datastore->recordExists(ZoneCluster, (uint8_t*)keyBuffer)) {
        logConsole("Cluster already exists");
        return false;
    }

    clusterBuffer.setClusterId(clusterId);
    clusterBuffer.setAlias(alias);
    clusterBuffer.setKey(symmetricKey);
    clusterBuffer.setIv(iv);
    char freq[CHATTER_LORA_FREQUENCY_DIGITS + 1];
    sprintf(freq, "%01d", frequency);

    clusterBuffer.setFrequency(freq);
    clusterBuffer.setWifiSsid(wifiSsid);
    clusterBuffer.setWifiCred(wifiCred);

    clusterBuffer.setPreferredChannel(preferredChannel);
    clusterBuffer.setSecondaryChannel(secondaryChannel);
    clusterBuffer.setStatus(ClusterActive);

    if (datastore->writeToNextSlot(&clusterBuffer)) {
        logConsole("Cluster added");
        return true;
    }

    logConsole("Cluster add failed");
    return false;
}
