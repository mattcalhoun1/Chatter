#if defined (ARDUINO_SAMD_NANO_33_IOT)

#include "ArduinoBLE.h"
#include "ClusterInterface.h"
#include "ClusterGlobals.h"

#ifndef BLECLUSTERINTERFACE_H
#define BLECLUSTERINTERFACE_H

class BleClusterInterface : public ClusterInterface {
    public:
        bool init ();
        bool handleClientInput (const char* input, int inputLength);
        bool sendToClient (const char* clusterData, int dataLength);
        bool isConnected ();
    protected:
        bool running = false;    
        bool connected = false;
        BLEService* service;
        BLECharacteristic* tx;
        BLECharacteristic* rx;
        BLEDevice bleDevice;


};

#endif

#endif