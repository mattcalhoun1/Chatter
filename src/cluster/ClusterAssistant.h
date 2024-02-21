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
    ClusterWifiSsid = 4,
    ClusterWifiCred = 5,
    ClusterFrequency = 6,
    ClusterTime = 7,
    ClusterPrimaryChannel = 8,
    ClusterSecondaryChannel = 9,
    ClusterAuth = 10,
    ClusterNone = 11
};

class ClusterAssistant : public ClusterManagerBase {
    public:
        ClusterAssistant(Chatter* _chatter) : ClusterManagerBase(_chatter) {}
        bool attemptOnboard ();

    protected:
        void sendOnboardRequest();
        void sendPublicKey(Hsm* hsm, Encryptor* encryptor);
        ClusterConfigType ingestClusterData (const char* dataLine, int bytesRead, TrustStore* trustStore, Encryptor* encryptor);
};

#endif