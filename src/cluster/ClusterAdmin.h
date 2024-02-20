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
        bool syncDevice (const char* deviceId, const char* alias);
        bool onboardNewDevice (ChatterDeviceType deviceType, const char* devicePublicKey); 

        bool dumpTruststore (); // writes to serial
        bool dumpSymmetricKey();
        bool dumpTime ();
        bool dumpWiFi ();
        bool dumpFrequency ();
        bool dumpDevice (const char* deviceId, const char* alias);
        AdminRequestType extractRequestType (const char* request);
        ChatterDeviceType extractDeviceType (const char* request);
};

#endif