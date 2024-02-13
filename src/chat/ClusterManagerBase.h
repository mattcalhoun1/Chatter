#include <Arduino.h>
#include "ChatGlobals.h"
#include "Chatter.h"

#ifndef CHATTERCLUSTERBASE_H
#define CHATTERCLUSTERBASE_H

#define CLUSTER_REQ_DELIMITER ':'
#define CLUSTER_REQ_SYNC "SYNC"
#define CLUSTER_REQ_ONBOARD "ONBD"
#define CLUSTER_REQ_GENESIS "GENE"

#define CLUSTER_CFG_DELIMITER ':'
#define CLUSTER_CFG_TRUST "TRUST"
#define CLUSTER_CFG_KEY "KEY"
#define CLUSTER_CFG_IV "IV"
#define CLUSTER_CFG_WIFI "WIFI"
#define CLUSTER_CFG_TIME "TIME"
#define CLUSTER_CFG_FREQ "FREQ"
#define CLUSTER_CFG_DEVICE "DEVICE"

class ClusterManagerBase {
    public:
        ClusterManagerBase(Chatter* _chatter) { chatter = _chatter; }
    protected:
        virtual bool ingestPublicKey (byte* buffer);
        virtual bool ingestSymmetricKey (byte* buffer);
        virtual bool getUserInput (const char* prompt, char* inputBuffer, int minLength, int maxLength, bool symbolsAllowed, bool lowerCaseAllowed);

        Chatter* chatter;
};

#endif