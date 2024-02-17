#include "../chat/ChatGlobals.h"
#include "ClusterManagerBase.h"
#include "../chat/Chatter.h"

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
        void sendPublicKey(Encryptor* encryptor);
        ClusterConfigType ingestClusterData (const char* dataLine, int bytesRead, Encryptor* encryptor);
};

#endif