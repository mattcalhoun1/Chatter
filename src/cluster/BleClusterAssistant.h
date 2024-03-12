#include "ClusterAssistant.h"
#include "ClusterAssistantInterface.h"
#include "ClusterGlobals.h"
#include <ArduinoBLE.h>
#include "BleBuffer.h"

#ifndef BLECLUSTERASSISTANT_H
#define BLECLUSTERASSISTANT_H

class BleClusterAssistant : public ClusterAssistantInterface {
    public:
        BleClusterAssistant(Chatter* _chatter) : ClusterAssistantInterface(_chatter) {}
        bool init ();
        bool attemptOnboard ();

    protected:
        bool connectToPeripheral();
        bool sendOnboardRequest();
        bool establishBleBuffer ();
        bool sendPublicKeyAndAlias(Hsm* hsm, Encryptor* encryptor, const char* deviceAlias);
        bool waitForPrompt (const char* expectedPrompt, int maxWaitTime);

        BLEDevice peripheral;
        BleBuffer* bleBuffer;
        //char configType[BLE_SMALL_BUFFER_SIZE];        

        // dont directly use these references for sending/receiving,
        // they are just so the objects dont go out of scope during processing
        BLECharacteristic rxObj;
        BLECharacteristic txObj;
        BLECharacteristic rxReadObj;
        BLECharacteristic txReadObj;
        BLECharacteristic status;

};

#endif