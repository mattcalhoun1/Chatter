#include <Arduino.h>
#include "../chat/ChatGlobals.h"
#include "../chat/Chatter.h"

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
#define CLUSTER_CFG_WIFI_SSID "WIFI"
#define CLUSTER_CFG_WIFI_CRED "CRED"
#define CLUSTER_CFG_TIME "TIME"
#define CLUSTER_CFG_FREQ "FREQ"
#define CLUSTER_CFG_DEVICE "DEVICE"
#define CLUSTER_CFG_PRIMARY "PRIMARY"
#define CLUSTER_CFG_SECONDARY "SECONDARY"
#define CLUSTER_CFG_AUTH "AUTH"

// prefix indicating the presence of a client's public key
#define CLUSTER_REQ_PUB_PREFIX "PUB:"

class ClusterManagerBase {
    public:
        ClusterManagerBase(Chatter* _chatter) { chatter = _chatter; }
    protected:
        virtual bool ingestPublicKey (byte* buffer);
        virtual bool ingestSymmetricKey (byte* buffer);
        virtual bool getUserInput (const char* prompt, char* inputBuffer, int minLength, int maxLength, bool symbolsAllowed, bool lowerCaseAllowed);

        Chatter* chatter;

        char newDeviceId[CHATTER_DEVICE_ID_SIZE + 1];
        char clusterId[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE + 1];

        char trustedDeviceId[CHATTER_DEVICE_ID_SIZE + 1];
        char alias[CHATTER_ALIAS_NAME_SIZE + 1];
        char hexEncodedPubKey[ENC_PUB_KEY_SIZE * 2 + 1];
        uint8_t pubKey[ENC_PUB_KEY_SIZE];

        char hexEncodedSymmetricKey[ENC_SYMMETRIC_KEY_SIZE * 2 + 1];
        uint8_t symmetricKey[ENC_SYMMETRIC_KEY_SIZE];

        char hexEncodedIv[ENC_IV_SIZE * 2 + 1];
        uint8_t iv[ENC_IV_SIZE];

        char loraFrequency[CHATTER_LORA_FREQUENCY_DIGITS + 1];
        float frequency;

        char wifiSsid[CHATTER_WIFI_STRING_MAX_SIZE + 1];
        char wifiCred[CHATTER_WIFI_STRING_MAX_SIZE + 1];

        ClusterChannel primaryChannel;
        ClusterChannel secondaryChannel;
        ClusterAuthType authType;

        void logConsole(const char* msg);
};

#endif