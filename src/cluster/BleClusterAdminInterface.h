//#if defined (ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_UNOR4_WIFI)

#include "ArduinoBLE.h"
#include "ClusterAdminInterface.h"
#include "ClusterGlobals.h"
#include "../chat/Chatter.h"
#include "BleBuffer.h"

#ifndef BLECLUSTERINTERFACE_H
#define BLECLUSTERINTERFACE_H

class BleClusterAdminInterface : public ClusterAdminInterface {
    public:
        BleClusterAdminInterface(Chatter* _chatter) : ClusterAdminInterface(_chatter) {}
        bool init ();
        bool handleClientInput (const char* input, int inputLength, BLEDevice* device);
        bool isConnected ();
    protected:
        bool ingestPublicKey (byte* buffer, BLEDevice* device);
        BleBuffer* bleBuffer;

        bool running = false;    
        bool connected = false;
        BLEService* service;
        BLECharacteristic* tx;
        BLECharacteristic* txRead;
        BLECharacteristic* rx;
        BLECharacteristic* rxRead;
        BLECharacteristic* status;
        BLEDevice bleDevice;

        uint8_t statusBuffer[BLE_SMALL_BUFFER_SIZE+1];
        uint8_t statusBufferLength = 0;

        // lower this
        int maxSessionStepWait = 60000;// dont wait more than this number of millis on any step
        int maxSessionDuration = 60000;// dont wait more than this number of millis on any step

        //bool syncDevice (const char* hostClusterId, const char* deviceId, const char* alias);
        //bool onboardNewDevice (const char* hostClusterId, ChatterDeviceType deviceType, const uint8_t* devicePublicKey); 

        bool dumpTruststore (const char* hostClusterId); 
        bool dumpSymmetricKey(const char* hostClusterId);
        bool dumpTime ();
        bool dumpWiFi (const char* hostClusterId);
        bool dumpFrequency (const char* hostClusterId);
        bool dumpChannels (const char* hostClusterId);
        bool dumpAuthType (const char* hostClusterId);
        bool dumpDevice (const char* deviceId, const char* alias);
        bool dumpLicense (const char* deviceId);
};

//#endif

#endif