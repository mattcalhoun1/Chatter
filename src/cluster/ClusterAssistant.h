#include "../chat/ChatGlobals.h"
#include "ClusterManagerBase.h"
#include "../chat/Chatter.h"
#include "../storage/TrustStore.h"
#include "../storage/DeviceStore.h"
#include "../storage/ClusterStore.h"
#include "../encryption/Hsm.h"


#ifndef CLUSTERASSISTANT_H
#define CLUSTERASSISTANT_H

enum ClusterConfigType {
    ClusterDeviceId = 0,
    ClusterTrustStore = 1,
    ClusterKey = 2,
    ClusterIv = 3,
    ClusterWifi = 4,
    ClusterFrequency = 5,
    ClusterTime = 6,
    ClusterNone = 7
};

class ClusterAssistant : public ClusterManagerBase {
    public:
        ClusterAssistant(Chatter* _chatter) : ClusterManagerBase(_chatter) {}
        bool attemptOnboard ();

    protected:
        void sendOnboardRequest();
        void sendPublicKey(Hsm* hsm);
        ClusterConfigType ingestClusterData (const char* dataLine, int bytesRead, ClusterStore* clusterStore);

        char newDeviceId[CHATTER_DEVICE_ID_SIZE + 1];
        char clusterId[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE + 1];

        char trustedDeviceId[CHATTER_DEVICE_ID_SIZE + 1];
        char alias[CHATTER_ALIAS_NAME_SIZE + 1];
        char trustedDeviceId[CHATTER_DEVICE_ID_SIZE + 1];
        char hexEncodedPubKey[ENC_PUB_KEY_SIZE * 2 + 1];

};

#endif