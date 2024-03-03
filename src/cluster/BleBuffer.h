#include <ArduinoBLE.h>
#include <Arduino.h>
#include "ClusterGlobals.h"

#ifndef BLE_BUFFER_H
#define BLE_BUFFER_H

class BleBuffer {
    public:
        BleBuffer (BLECharacteristic* _tx, BLECharacteristic* _txRead, BLECharacteristic* _rx, BLECharacteristic* _rxRead) {tx = _tx; txRead = _txRead; rx = _rx; rxRead = _rxRead; }

        bool sendBufferToClient (BLECharacteristic* bleChar, BLECharacteristic* bleFlagChar, uint8_t* buff, uint8_t len);
        uint8_t receiveBufferFromClient (BLECharacteristic* bleChar, BLECharacteristic* bleFlagChar, uint8_t* buff, uint8_t maxLength, BLEDevice* device);
        
        bool sendTxBufferToClient ();
        bool receiveRxBufferFromClient (BLEDevice* device);

        bool send (const char* content);

        uint8_t* getTxBuffer () {return txBuffer;}
        uint8_t getTxBufferLength () { return txBufferLength; }
        void setTxBufferLength (uint8_t len) {txBufferLength = len;}
        uint8_t* getRxBuffer () {return rxBuffer;}
        uint8_t getRxBufferLength () { return rxBufferLength; }
        void setRxBufferLength (uint8_t len) {rxBufferLength = len;}

        void clearTxBuffer ();
        void clearRxBuffer ();
    
        uint8_t findChar (char toFind, const  uint8_t* buffer, uint8_t bufferLen);

    protected:
        bool bleBufferContainsHeader ();
        bool bleBufferContainsFooter ();
        bool writeBleBufferWait (BLECharacteristic* bleChar, BLECharacteristic* bleFlagChar, uint8_t* buff, int len);

        BLECharacteristic* tx;
        BLECharacteristic* txRead;
        BLECharacteristic* rx;
        BLECharacteristic* rxRead;

        uint8_t rxBuffer[BLE_MAX_BUFFER_SIZE+1];
        uint8_t rxBufferLength = 0;
        uint8_t txBuffer[BLE_MAX_BUFFER_SIZE+1];
        uint8_t txBufferLength = 0;
        //uint8_t statusBuffer[BLE_SMALL_BUFFER_SIZE+1];
        //uint8_t statusBufferLength = 0;

        uint8_t bleBuffer[BLE_SMALL_BUFFER_SIZE]; // holds pieces of transmissions
        uint8_t bleBufferLength = 0;

        void logConsole (const char* msg);

};

#endif