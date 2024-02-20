#include "BleClusterInterface.h"

#if defined (ARDUINO_SAMD_NANO_33_IOT)
bool BleClusterInterface::init () {
    service = new BLEService(BLE_CLUSTER_INTERFACE_UUID);
    tx = new BLECharacteristic(BLE_CLUSTER_CHAR_UUID, BLERead, 150);
    rx = new BLECharacteristic(BLE_CLUSTER_CHAR_UUID, BLEWrite, 150);

    service->addCharacteristic(*tx);
    service->addCharacteristic(*rx);

    
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
    Serial.print("Client Input: ");
    for (int i = 0; i < inputLength; i++) {
        Serial.print(input[i]);
    }
    Serial.println("");
    return true;
}

bool BleClusterInterface::sendToClient (const char* clusterData, int dataLength) {
    return true;
}

bool BleClusterInterface::isConnected () {
    connected = false;
    bleDevice = BLE.central();
    if (bleDevice) {
        Serial.print("Connected to central: ");
        // print the central's MAC address:
        Serial.println(bleDevice.address());    
        connected = true;

        if (rx->written()) {
            Serial.println("characteristic changed.");
        }

        uint8_t txBuffer[128];
        for (int i = 0; i < 128; i++) {
            txBuffer[i] = i;
        }
        tx->setValue((const char*)txBuffer);
    }
    return connected;
}

#endif