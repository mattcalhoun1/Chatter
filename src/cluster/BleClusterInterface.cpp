#include "BleClusterInterface.h"

#if defined (ARDUINO_SAMD_NANO_33_IOT)
bool BleClusterInterface::init () {
    service = new BLEService(BLE_CLUSTER_INTERFACE_UUID);
    byteCharacteristic = new BLEByteCharacteristic(BLE_CLUSTER_CHAR_UUID, BLERead | BLEWrite);
    service->addCharacteristic(*byteCharacteristic);
    
    if(BLE.begin()) {
        BLE.setDeviceName(BLE_CLUSTER_SERVICE_NAME);
        BLE.addService(*service);
        BLE.setLocalName(BLE_CLUSTER_SERVICE_NAME);
        BLE.setAdvertisedService(*service);


        byte data[5] = { 0x01, 0x02, 0x03, 0x04, 0x05};
        BLE.setManufacturerData(data, 5);

        BLE.setAdvertisingInterval(320); // 200 * 0.625 ms
        // start advertising
        BLE.advertise();

        running = true;
    }
    else {
        Serial.println("Error BLE.begin()");
        return false;
    }

    return running;
}

bool BleClusterInterface::handleClientInput (const char* input, int inputLength) {
    return true;
}

bool BleClusterInterface::sendToClient (const char* clusterData, int dataLength) {
    return true;
}

bool BleClusterInterface::isConnected () {
    return connected;
}

#endif