//#if defined (ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_UNOR4_WIFI)

#include "ArduinoBLE.h"
#include "ClusterAdminInterface.h"
#include "ClusterGlobals.h"
#include "../chat/Chatter.h"

#ifndef BLECLUSTERINTERFACE_H
#define BLECLUSTERINTERFACE_H

#define BLE_MAX_BUFFER_SIZE 150
#define BLE_SMALL_BUFFER_SIZE 32

class BleClusterAdminInterface : public ClusterAdminInterface {
    public:
        BleClusterAdminInterface(Chatter* _chatter) : ClusterAdminInterface(_chatter) {}
        bool init ();
        bool handleClientInput (const char* input, int inputLength);
        bool isConnected ();
    protected:
        bool sendTxBufferToClient (const char* expectedConfirmation);
        bool ingestPublicKey (byte* buffer);

        bool running = false;    
        bool connected = false;
        BLEService* service;
        BLECharacteristic* tx;
        BLECharacteristic* rx;
        BLECharacteristic* status;
        BLEDevice bleDevice;

        uint8_t rxBuffer[BLE_MAX_BUFFER_SIZE+1];
        uint8_t rxBufferLength = 0;
        uint8_t txBuffer[BLE_MAX_BUFFER_SIZE+1];
        uint8_t txBufferLength = 0;
        uint8_t statusBuffer[BLE_SMALL_BUFFER_SIZE+1];
        uint8_t statusBufferLength = 0;

        // lower this
        int maxSessionStepWait = 60000;// dont wait more than this number of millis on any step

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
};

//#endif

#endif