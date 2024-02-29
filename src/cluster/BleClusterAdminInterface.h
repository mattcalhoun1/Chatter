//#if defined (ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_UNOR4_WIFI)

#include "ArduinoBLE.h"
#include "ClusterAdminInterface.h"
#include "ClusterGlobals.h"
#include "../chat/Chatter.h"

#ifndef BLECLUSTERINTERFACE_H
#define BLECLUSTERINTERFACE_H

#define BLE_MAX_BUFFER_SIZE 150
#define BLE_SMALL_BUFFER_SIZE 14

class BleClusterAdminInterface : public ClusterAdminInterface {
    public:
        BleClusterAdminInterface(Chatter* _chatter) : ClusterAdminInterface(_chatter) {}
        bool init ();
        bool handleClientInput (const char* input, int inputLength, BLEDevice* device);
        bool isConnected ();
    protected:
        bool sendBufferToClient (BLECharacteristic* bleChar, BLECharacteristic* bleFlagChar, uint8_t* buff, uint8_t len);
        uint8_t receiveBufferFromClient (BLECharacteristic* bleChar, BLECharacteristic* bleFlagChar, uint8_t* buff, uint8_t maxLength, BLEDevice* device);
        bool sendTxBufferToClient ();
        bool receiveRxBufferFromClient (BLEDevice* device);

        bool writeBleBufferWait (BLECharacteristic* bleChar, BLECharacteristic* bleFlagChar, uint8_t* buff, int len);
        bool bleBufferContainsHeader ();
        bool bleBufferContainsFooter ();

        bool ingestPublicKey (byte* buffer, BLEDevice* device);

        bool running = false;    
        bool connected = false;
        BLEService* service;
        BLECharacteristic* tx;
        BLECharacteristic* txRead;
        BLECharacteristic* rx;
        BLECharacteristic* rxRead;
        BLECharacteristic* status;
        BLEDevice bleDevice;

        uint8_t rxBuffer[BLE_MAX_BUFFER_SIZE+1];
        uint8_t rxBufferLength = 0;
        uint8_t txBuffer[BLE_MAX_BUFFER_SIZE+1];
        uint8_t txBufferLength = 0;
        uint8_t statusBuffer[BLE_SMALL_BUFFER_SIZE+1];
        uint8_t statusBufferLength = 0;

        uint8_t bleBuffer[BLE_SMALL_BUFFER_SIZE]; // holds pieces of transmissions
        uint8_t bleBufferLength = 0;

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

        uint8_t findChar (char toFind, const uint8_t* buffer, uint8_t bufferLen);
};

//#endif

#endif