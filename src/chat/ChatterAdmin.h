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
        bool handleAdminRequest ();

    private:
        bool genesis ();
        bool syncDevice (const char* deviceId, const char* alias);
        bool onboardNewDevice (ChatterDeviceType deviceType, const char* devicePublicKey); 

        bool dumpTruststore (); // writes to serial
        bool dumpSymmetricKey();
        bool dumpTime ();
        bool dumpWiFi ();
        bool dumpDevice (const char* deviceId, const char* alias);
        AdminRequestType extractRequestType (const char* request);
        ChatterDeviceType extractDeviceType (const char* request);

        bool ingestPublicKey (byte* buffer);
        bool getUserInput (const char* prompt, char* inputBuffer, int minLength, int maxLength, bool symbolsAllowed, bool lowerCaseAllowed);

        Chatter* chatter;
};

#endif