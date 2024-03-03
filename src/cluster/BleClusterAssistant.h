#include "ClusterAssistant.h"
#include "ClusterGlobals.h"
#include <ArduinoBLE.h>

#ifndef BLECLUSTERASSISTANT_H
#define BLECLUSTERASSISTANT_H

class BleClusterAssistant : public ClusterAssistant {
    public:
        BleClusterAssistant(Chatter* _chatter) : ClusterAssistant(_chatter) {}
        bool init ();
        bool attemptOnboard ();

    protected:
        void sendOnboardRequest();
        void sendPublicKey(Hsm* hsm, Encryptor* encryptor);
        ClusterConfigType ingestClusterData (const char* dataLine, int bytesRead, TrustStore* trustStore, Encryptor* encryptor);

        uint8_t txBuffer[BLE_MAX_BUFFER_SIZE + 1];
        uint8_t txBufferLength = 0;
        uint8_t rxBuffer[BLE_SMALL_BUFFER_SIZE + 1];
        uint8_t rxBufferLength = 0;
        uint8_t bleBuffer[BLE_SMALL_BUFFER_SIZE + 1];
        uint8_t bleBufferLength = 0;
        char configType[BLE_SMALL_BUFFER_SIZE];        
};

#endif