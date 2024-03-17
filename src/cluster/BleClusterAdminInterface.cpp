#include "BleClusterAdminInterface.h"

//#if defined (ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_UNOR4_WIFI)
bool BleClusterAdminInterface::init () {
    service = new BLEService(BLE_CLUSTER_INTERFACE_UUID);

    // for data from admin to other device
    tx = new BLECharacteristic(BLE_CLUSTER_TX_UUID, BLERead, BLE_SMALL_BUFFER_SIZE, true);
    txRead = new BLECharacteristic(BLE_CLUSTER_TX_READ_UUID, BLERead | BLEWrite, 1);

    // from other device to admin
    rx = new BLECharacteristic(BLE_CLUSTER_RX_UUID, BLERead | BLEWrite, BLE_SMALL_BUFFER_SIZE, true);
    rxRead = new BLECharacteristic(BLE_CLUSTER_RX_READ_UUID, BLERead | BLEWrite, 1);

    status = new BLECharacteristic(BLE_CLUSTER_STATUS_UUID, BLERead, BLE_SMALL_BUFFER_SIZE, true);

    service->addCharacteristic(*tx);
    service->addCharacteristic(*rx);
    service->addCharacteristic(*txRead);
    service->addCharacteristic(*rxRead);
    service->addCharacteristic(*status);

    
    if(BLE.begin()) {
        BLE.setDeviceName(BLE_CLUSTER_ADMIN_SERVICE_NAME);
        BLE.addService(*service);
        BLE.setLocalName(BLE_CLUSTER_ADMIN_SERVICE_NAME);
        BLE.setAdvertisedService(*service);


        byte data[5] = { 0x01, 0x02, 0x03, 0x04, 0x05};
        BLE.setManufacturerData(data, 5);

        BLE.setAdvertisingInterval(320); // 200 * 0.625 ms

        bleBuffer = new BleBuffer(tx, txRead, rx, rxRead);

        // write status values to characteristic. dont use ble buffer, since we are not connected to a device yet
        uint8_t* txBuffer = bleBuffer->getTxBuffer();
        memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
        sprintf((char*)txBuffer, "%s", "ready");
        rx->writeValue(txBuffer, BLE_SMALL_BUFFER_SIZE);
        tx->writeValue(txBuffer, BLE_SMALL_BUFFER_SIZE);

        BLE.advertise();

        running = true;
    }
    else {
        logConsole("Error BLE.begin()");
        return false;
    }

    return running;
}

bool BleClusterAdminInterface::handleClientInput (const char* input, int inputLength, BLEDevice* device) {
    logConsole("Extracting types");
    AdminRequestType requestType = extractRequestType(input);
    ChatterDeviceType deviceType = extractDeviceType(input);

    logConsole("Admin request type: " + String(requestType) + " from device type: " + String(deviceType));// + " Received " + input);
    if (requestType == AdminRequestOnboard || requestType == AdminRequestSync) {
        // pub key is required
        memset(alias, 0, CHATTER_ALIAS_NAME_SIZE + 1);
        if (ingestPublicKeyAndAlias(chatter->getEncryptor()->getPublicKeyBuffer(), alias, device)) {

            // future enhancement: this would be a good point to generate a message to challenge for a signature. 

            // check if this device id is known
            char knownDeviceId[CHATTER_DEVICE_ID_SIZE+1];
            char knownAlias[CHATTER_ALIAS_NAME_SIZE+1];
            bool trustedKey = chatter->getTrustStore()->findDeviceId ((const uint8_t*)chatter->getEncryptor()->getPublicKeyBuffer(), chatter->getClusterId(), knownDeviceId);
            if(trustedKey) {
                chatter->getTrustStore()->loadAlias(knownDeviceId, knownAlias);
                Serial.print("We Know: ");
                Serial.print(knownDeviceId);
                Serial.print("/");
                Serial.println(knownAlias);

                // if it's onboard, it should not yet exist in our truststore. if that's the case, do a sync instead
                // if it's sync, we are good to proceed
                return syncDevice(chatter->getClusterId(), knownDeviceId, knownAlias);
            } 
            else {
                // we don't know this key, which is to be expected if it's on onboard
                if (requestType == AdminRequestOnboard) {
                    logConsole("this is a new device, onboarding");
                    return onboardNewDevice(chatter->getClusterId(), deviceType, (const uint8_t*)chatter->getEncryptor()->getPublicKeyBuffer(), alias);
                }
            }
        }
        else {
            logConsole("Bad public key!");
            return false;
        }
    }
    else {
        logConsole("Invalid request type");
        return false;
    }

    return true;
}


bool BleClusterAdminInterface::isConnected () {
    connected = false;
    bleDevice = BLE.central();
    if (bleDevice && bleDevice.connected()) {
        logConsole("Connected to central: ");
        // print the central's MAC address:
        logConsole(bleDevice.address());    
        String devAddr = bleDevice.address();
        connected = true;

        // this takes over all processing until the connection ends
        bool success = true;
        unsigned long connStartTime = millis();

        // wait for client to finish
        unsigned long startTime = millis();
        while (millis() - startTime < BLE_CONNECT_TIME) {
            delay(500);
            bleDevice.poll();
        }

        while (bleDevice.connected() && success) {
            if (bleBuffer->receiveRxBufferFromClient(&bleDevice)) {
                if (bleBuffer->findChar(CLUSTER_CFG_DELIMITER, bleBuffer->getRxBuffer(), bleBuffer->getRxBufferLength()) != 255) {
                    logConsole("handling client input");
                    if (handleClientInput((const char*)bleBuffer->getRxBuffer(), bleBuffer->getRxBufferLength(), &bleDevice)) {
                        logConsole("Client successfully handled");
                        return true;
                    }
                    else {
                        delay(500);
                    }
                }
                else {
                    delay(500);
                }
            }
            else if (!bleDevice.connected()) {
                logConsole("disconnected while attempting to receive rx buffer");
                success = false;
            }

            // timeout if taken too long
            if (millis() - connStartTime > maxSessionDuration) {
                bleBuffer->send("ERROR");
            }
        }

        logConsole("BLE device no longer connected");

        // ready for new connections
        memset (statusBuffer, 0, BLE_SMALL_BUFFER_SIZE);
        memcpy(statusBuffer, "ready",5);
        status->writeValue(statusBuffer, BLE_SMALL_BUFFER_SIZE);
    }
    return connected;
}

bool BleClusterAdminInterface::ingestPublicKeyAndAlias (byte* pubKeyBuffer, char* aliasBuffer, BLEDevice* bleDevice) {
    bool validKey = true; // assume valid until we find a bad byte
    unsigned long startTime = millis();

    if (bleBuffer->send("PUB")) {
        // pub key can only be hex with no spaces
        memset(hexEncodedPubKey, 0, ENC_PUB_KEY_SIZE * 2 + 1);

        // receive key bytes from  client
        if(bleBuffer->receiveRxBufferFromClient(bleDevice)) {
            uint8_t* rxBuffer = bleBuffer->getRxBuffer();
            uint8_t rxBufferLength = bleBuffer->getRxBufferLength();

            if (memcmp(rxBuffer, "PUB:", 4) == 0) {
                memcpy(hexEncodedPubKey, rxBuffer + 4, rxBufferLength - 4);

                for (uint8_t i = 4; i < 4 + (ENC_PUB_KEY_SIZE * 2); i++) {
                    char currChar = rxBuffer[i];
                    if ((currChar >= '0' && currChar <= '9') || (currChar >= 'A' && currChar <= 'Z')) {
                        // this looks good;
                    }
                    else {
                        Serial.print("Bad pub key character : "); Serial.println(currChar);
                        validKey = false;
                    }
                }

                if (rxBufferLength > (ENC_PUB_KEY_SIZE * 2) + 4 && validKey) {
                    logConsole("Public key appears valid");

                    // make sure alias is valid
                    if (rxBufferLength >= (ENC_PUB_KEY_SIZE * 2) + 4 + CHATTER_MIN_ALIAS_LENGTH) {
                        // decode the pub key into the provided buffer
                        chatter->getEncryptor()->hexCharacterStringToBytesMax(pubKeyBuffer, (const char*)rxBuffer + 4, ENC_PUB_KEY_SIZE * 2, ENC_PUB_KEY_SIZE);

                        // the remaining rx buffer is the device alias
                        memset(aliasBuffer, 0, CHATTER_ALIAS_NAME_SIZE);
                        memcpy(aliasBuffer, (const char*)rxBuffer + 4 + ENC_PUB_KEY_SIZE*2, rxBufferLength - ((ENC_PUB_KEY_SIZE*2) + 4));

                        Serial.print("Device Alias: "); Serial.println(aliasBuffer);
                        return true;
                    }
                    else {
                        logConsole("alias is too short!");
                    }
                }
                else {
                    logConsole("Public key received appears invalid");
                }
            }
            else {
                Serial.print("Val read, not pub key: "); Serial.println((const char*)rxBuffer);
            }
        }
        else {
            logConsole("Pub key not received");
        }
    }
    else {
        logConsole("pub key request not acknowledged");
    }

    return false;
}


bool BleClusterAdminInterface::dumpTruststore (const char* hostClusterId) {
    TrustStore* trustStore = chatter->getTrustStore();
    Encryptor* encryptor = chatter->getEncryptor();

    List<String> others = trustStore->getDeviceIds();
    char otherDeviceAlias[CHATTER_ALIAS_NAME_SIZE + 1];
    for (int i = 0; i < others.getSize(); i++) {
        const String& otherDeviceStr = others[i];
        // if its pub key of device on this cluser, dump it
        const char* otherDeviceId = otherDeviceStr.c_str();
        if (memcmp(otherDeviceId, hostClusterId, CHATTER_GLOBAL_NET_ID_SIZE + CHATTER_LOCAL_NET_ID_SIZE) == 0) {
            trustStore->loadAlias(otherDeviceId, otherDeviceAlias);
            trustStore->loadPublicKey(otherDeviceId, pubKey);

            encryptor->hexify(pubKey, ENC_PUB_KEY_SIZE);
            memset(pubKey, 0, ENC_PUB_KEY_SIZE);

            bleBuffer->clearTxBuffer();
            memcpy(bleBuffer->getTxBuffer(), 0, BLE_MAX_BUFFER_SIZE+1);
            sprintf((char*)bleBuffer->getTxBuffer(), "%s%c%s%s%s", CLUSTER_CFG_TRUST, CLUSTER_CFG_DELIMITER, otherDeviceId, (char*)encryptor->getHexBuffer(), otherDeviceAlias);
            bleBuffer->setTxBufferLength(strlen(CLUSTER_CFG_TRUST) + 1 + CHATTER_DEVICE_ID_SIZE + ENC_PUB_KEY_SIZE*2 + strlen(otherDeviceAlias));

            if (bleBuffer->sendTxBufferToClient()) {
                logConsole("Trust sent");
            }
            else {
                logConsole("Trust send failed");
            }
        }
    }
    return true;
}

bool BleClusterAdminInterface::dumpSymmetricKey(const char* hostClusterId) {
    chatter->getClusterStore()->loadSymmetricKey(hostClusterId, symmetricKey);

    Encryptor* encryptor = chatter->getEncryptor();
    encryptor->hexify(symmetricKey, ENC_SYMMETRIC_KEY_SIZE);
    memset(symmetricKey, 0, ENC_SYMMETRIC_KEY_SIZE*2);

    uint8_t bufferPos = 0;
    bleBuffer->clearTxBuffer();
    uint8_t* txBuffer = bleBuffer->getTxBuffer();
    memcpy(txBuffer, CLUSTER_CFG_KEY, strlen(CLUSTER_CFG_KEY));
    bufferPos += strlen(CLUSTER_CFG_KEY);
    txBuffer[bufferPos++] = CLUSTER_CFG_DELIMITER;
    memcpy(txBuffer + bufferPos, encryptor->getHexBuffer(), ENC_SYMMETRIC_KEY_SIZE*2);
    bleBuffer->setTxBufferLength(strlen(CLUSTER_CFG_KEY) + 1 + ENC_SYMMETRIC_KEY_SIZE * 2);

    if (bleBuffer->sendTxBufferToClient()) {
        logConsole("Symmetric key sent");
    }
    else {
        logConsole("Symmetric key send failed");
        return false;
    }

    chatter->getClusterStore()->loadIv(hostClusterId, iv);
    encryptor->hexify(iv, ENC_IV_SIZE);
    memset(iv, 0, ENC_IV_SIZE);

    bufferPos = 0;
    bleBuffer->clearTxBuffer();
    memcpy(txBuffer, CLUSTER_CFG_IV, strlen(CLUSTER_CFG_IV));
    bufferPos += strlen(CLUSTER_CFG_IV);
    txBuffer[bufferPos++] = CLUSTER_CFG_DELIMITER;
    memcpy(txBuffer + bufferPos, encryptor->getHexBuffer(), ENC_IV_SIZE*2);
    encryptor->clearHexBuffer(); // dont leave key in hex buffer
    bleBuffer->setTxBufferLength(strlen(CLUSTER_CFG_IV) + 1 + ENC_IV_SIZE*2);

    if (bleBuffer->sendTxBufferToClient()) {
        logConsole("Iv sent");
    }
    else {
        logConsole("Iv send failed");
        return false;
    }

    return true;
}

bool BleClusterAdminInterface::dumpWiFi (const char* hostClusterId) {
    // CLUSTER_CFG_WIFI_SSID , CLUSTER_CFG_WIFI_CRED
    chatter->getClusterStore()->loadWifiSsid(hostClusterId, wifiSsid);
    chatter->getClusterStore()->loadWifiCred(hostClusterId, wifiCred);

    bleBuffer->clearTxBuffer();
    uint8_t* txBuffer = bleBuffer->getTxBuffer();
    sprintf((char*)txBuffer, "%s%c%s", CLUSTER_CFG_WIFI_SSID, CLUSTER_CFG_DELIMITER, wifiSsid);
    bleBuffer->setTxBufferLength(strlen(CLUSTER_CFG_WIFI_SSID) + 1 + strlen(wifiSsid));
    if(bleBuffer->sendTxBufferToClient()) {
        logConsole("Wifi SSID sent");

        bleBuffer->clearTxBuffer();
        bleBuffer->setTxBufferLength(strlen(CLUSTER_CFG_WIFI_CRED) + 1 + strlen(wifiCred));
        sprintf((char*)txBuffer, "%s%c%s", CLUSTER_CFG_WIFI_CRED, CLUSTER_CFG_DELIMITER, wifiCred);
        if(bleBuffer->sendTxBufferToClient()) {
            logConsole("Wifi cred sent");
        }
        else {
            logConsole("Wifi cred not sent");
            return false;
        }

    }
    else {
        logConsole("Wifi SSID not sent");
        return false;
    }

    return true;
}

bool BleClusterAdminInterface::dumpFrequency (const char* hostClusterId) {
    bleBuffer->clearTxBuffer();
    sprintf((char*)bleBuffer->getTxBuffer(), "%s%c%01f", CLUSTER_CFG_FREQ, CLUSTER_CFG_DELIMITER, chatter->getClusterStore()->getFrequency(hostClusterId));
    bleBuffer->setTxBufferLength(strlen(CLUSTER_CFG_FREQ) + 1 + 5);
    if (bleBuffer->sendTxBufferToClient()) {
        logConsole("Lora freq sent");
    }
    else {
        logConsole("Lora freq not sent");
        return false;
    }
    return true;
}

bool BleClusterAdminInterface::dumpChannels (const char* hostClusterId) {
    bleBuffer->clearTxBuffer();
    sprintf((char*)bleBuffer->getTxBuffer(), "%s%c%s", CLUSTER_CFG_PRIMARY, CLUSTER_CFG_DELIMITER, (char)chatter->getClusterStore()->getPreferredChannel(hostClusterId));
    bleBuffer->setTxBufferLength(strlen(CLUSTER_CFG_PRIMARY) + 1 + 1);

    if (bleBuffer->sendTxBufferToClient()) {
        logConsole("Primary channel sent");

        bleBuffer->clearTxBuffer();
        sprintf((char*)bleBuffer->getTxBuffer(), "%s%c%s", CLUSTER_CFG_SECONDARY, CLUSTER_CFG_DELIMITER, (char)chatter->getClusterStore()->getSecondaryChannel(hostClusterId));
        bleBuffer->setTxBufferLength(strlen(CLUSTER_CFG_SECONDARY) + 1 + 1);
        if (bleBuffer->sendTxBufferToClient()) {
            logConsole("Secondary channel sent");
        }
        else {
            logConsole("Secondary channel not sent");
            return false;
        }
    }
    else {
        logConsole("Primary channel not sent");
        return false;
    }

    return true;
}

bool BleClusterAdminInterface::dumpAuthType (const char* hostClusterId) {
    bleBuffer->clearTxBuffer();
    sprintf((char*)bleBuffer->getTxBuffer(), "%s%c%s", CLUSTER_CFG_AUTH, CLUSTER_CFG_DELIMITER, (char)chatter->getClusterStore()->getAuthType(hostClusterId));
    bleBuffer->setTxBufferLength(strlen(CLUSTER_CFG_AUTH) + 1 + 1);

    if (bleBuffer->sendTxBufferToClient()) {
        logConsole("Auth type sent");
    }
    else {
        logConsole("Auth type not sent");
        return false;
    }

    return true;
}


bool BleClusterAdminInterface::dumpTime () {
    bleBuffer->clearTxBuffer();
    sprintf((char*)bleBuffer->getTxBuffer(), "%s%c%s", CLUSTER_CFG_TIME, CLUSTER_CFG_DELIMITER, chatter->getRtc()->getSortableTime());
    bleBuffer->setTxBufferLength(strlen(CLUSTER_CFG_TIME) + 1 + 12);

    if (bleBuffer->sendTxBufferToClient()) {
        logConsole("Time sent");
    }
    else {
        logConsole("Time not sent");
        return false;
    }

    return true;
}

bool BleClusterAdminInterface::dumpDeviceAndClusterAlias (const char* deviceId) {
    bleBuffer->clearTxBuffer();
    //sprintf((char*)bleBuffer->getTxBuffer(), "%s%c%s%s", CLUSTER_CFG_DEVICE, CLUSTER_CFG_DELIMITER, deviceId, alias);
    sprintf((char*)bleBuffer->getTxBuffer(), "%s%c%s%s", CLUSTER_CFG_DEVICE, CLUSTER_CFG_DELIMITER, deviceId, chatter->getClusterAlias());
    bleBuffer->setTxBufferLength(strlen(CLUSTER_CFG_DEVICE) + 1 + CHATTER_DEVICE_ID_SIZE + strlen(chatter->getClusterAlias()));

    if (bleBuffer->sendTxBufferToClient()) {
        logConsole("Device ID sent");
    }
    else {
        logConsole("Device ID not sent");
        return false;
    }

    return true;
}

bool BleClusterAdminInterface::dumpLicense (const char* deviceId, const char* deviceAlias) {
    if (generateEncodedLicense(deviceId, deviceAlias)) {
        bleBuffer->clearTxBuffer();
        sprintf((char*)bleBuffer->getTxBuffer(), "%s%c%s%s", CLUSTER_CFG_LICENSE, CLUSTER_CFG_DELIMITER, chatter->getDeviceId(), hexEncodedLicense);
        bleBuffer->setTxBufferLength(strlen(CLUSTER_CFG_LICENSE) + 1 + CHATTER_DEVICE_ID_SIZE + (ENC_SIGNATURE_SIZE * 2));

        if (bleBuffer->sendTxBufferToClient()) {
            logConsole("License sent");
            return true;
        }
        else {
            logConsole("License not sent, ble send failed");
        }

    }

    logConsole("license not sent, failed to generate");
    return false;
}
