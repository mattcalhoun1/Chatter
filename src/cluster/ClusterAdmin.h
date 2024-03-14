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
        bool genesis (const char* deviceAlias, const char* clusterAlias, float loraFrequency);
        bool genesisRandom (const char* deviceAlias);

    protected:
        bool syncDevice (const char* hostClusterId, const char* deviceId, const char* deviceAlias);
        bool onboardNewDevice (const char* hostClusterId, ChatterDeviceType deviceType, const uint8_t* devicePublicKey, const char* deviceAlias); 

        virtual bool dumpTruststore (const char* hostClusterId); 
        virtual bool dumpSymmetricKey(const char* hostClusterId);
        virtual bool dumpTime ();
        virtual bool dumpWiFi (const char* hostClusterId);
        virtual bool dumpFrequency (const char* hostClusterId);
        virtual bool dumpChannels (const char* hostClusterId);
        virtual bool dumpAuthType (const char* hostClusterId);
        virtual bool dumpDevice (const char* deviceId, const char* deviceAlias);
        virtual bool dumpLicense (const char* deviceId, const char* deviceAlias);
        AdminRequestType extractRequestType (const char* request);
        ChatterDeviceType extractDeviceType (const char* request);

        virtual bool generateEncodedLicense (const char* deviceId, const char* deviceAlias);
};

#endif