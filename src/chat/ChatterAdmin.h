#include "Chatter.h"
#include "ChatGlobals.h"
#include <List.hpp>

#ifndef CHATTERADMIN_H
#define CHATTERADMIN_H

#define CHATTER_ADMIN_DELIMITER ':'

#define CHATTER_ADMIN_REQ_SYNC "SYNC"
#define CHATTER_ADMIN_REQ_ONBOARD "ONBD"
#define CHATTER_ADMIN_REQ_GENESIS "GENE"

enum AdminRequestType {
    AdminRequestNone = 0,
    AdminRequestSync = 1,
    AdminRequestOnboard = 2,
    AdminRequestGenesis = 3
};

class ChatterAdmin {
    public:
        ChatterAdmin (Chatter* _chatter) {chatter = _chatter;}

        bool genesis ();
        bool syncDevice ();
        bool onboardNewDevice (); 
        bool handleAdminRequest ();

    private:
        bool dumpTruststore (); // writes to serial
        bool dumpSymmetricKey();
        bool dumpTime ();
        bool dumpWiFi ();
        AdminRequestType extractRequestType (const char* request);
        ChatterDeviceType extractDeviceType (const char* request);

        bool ingestPublicKey (byte* buffer);

        Chatter* chatter;
};

#endif