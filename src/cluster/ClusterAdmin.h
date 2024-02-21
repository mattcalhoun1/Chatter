#include "../chat/Chatter.h"
#include "../chat/ChatGlobals.h"
#include "ClusterManagerBase.h"
#include <List.hpp>
#include "../storage/TrustStore.h"
#include "../storage/DeviceStore.h"
#include "../storage/ClusterStore.h"

#ifndef CHATTERADMIN_H
#define CHATTERADMIN_H

enum AdminRequestType {
    AdminRequestNone = 0,
    AdminRequestSync = 1,
    AdminRequestOnboard = 2,
    AdminRequestGenesis = 3
};

class ClusterAdmin : public ClusterManagerBase {
    public:
        ClusterAdmin (Chatter* _chatter) : ClusterManagerBase (_chatter) {}
        bool handleAdminRequest ();
        bool genesis ();
        bool genesisRandom ();

    private:
        bool syncDevice (const char* hostClusterId, const char* deviceId, const char* alias);
        bool onboardNewDevice (const char* hostClusterId, ChatterDeviceType deviceType, const uint8_t* devicePublicKey); 

        bool dumpTruststore (const char* hostClusterId); // writes to serial
        bool dumpSymmetricKey(const char* hostClusterId);
        bool dumpTime ();
        bool dumpWiFi (const char* hostClusterId);
        bool dumpFrequency (const char* hostClusterId);
        bool dumpChannels (const char* hostClusterId);
        bool dumpAuthType (const char* hostClusterId);
        bool dumpDevice (const char* deviceId, const char* alias);
        AdminRequestType extractRequestType (const char* request);
        ChatterDeviceType extractDeviceType (const char* request);
};

#endif