#include "BleClusterAdminInterface.h"

//#if defined (ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_UNOR4_WIFI)
bool BleClusterAdminInterface::init () {
    service = new BLEService(BLE_CLUSTER_INTERFACE_UUID);
    tx = new BLEStringCharacteristic(BLE_CLUSTER_TX_UUID, BLERead | BLENotify, BLE_MAX_BUFFER_SIZE);
    rx = new BLEStringCharacteristic(BLE_CLUSTER_RX_UUID, BLERead | BLEWrite | BLENotify, BLE_MAX_BUFFER_SIZE);
    status = new BLEStringCharacteristic(BLE_CLUSTER_STATUS_UUID, BLERead, BLE_SMALL_BUFFER_SIZE);

    service->addCharacteristic(*tx);
    service->addCharacteristic(*rx);
    service->addCharacteristic(*status);

    
    if(BLE.begin()) {
        BLE.setDeviceName(BLE_CLUSTER_ADMIN_SERVICE_NAME);
        BLE.addService(*service);
        BLE.setLocalName(BLE_CLUSTER_ADMIN_SERVICE_NAME);
        BLE.setAdvertisedService(*service);


        byte data[5] = { 0x01, 0x02, 0x03, 0x04, 0x05};
        BLE.setManufacturerData(data, 5);

        BLE.setAdvertisingInterval(320); // 200 * 0.625 ms

        rx->writeValue("ready");
        tx->writeValue("ready");
        rx->subscribe();

        BLE.advertise();

        running = true;
    }
    else {
        Serial.println("Error BLE.begin()");
        return false;
    }

    return running;
}

bool BleClusterAdminInterface::handleClientInput (const char* input, int inputLength) {
    AdminRequestType requestType = extractRequestType(input);
    ChatterDeviceType deviceType = extractDeviceType(input);

    Serial.println("Admin request type: " + String(requestType) + " from device type: " + String(deviceType) + " Received " + input);
    if (requestType == AdminRequestOnboard || requestType == AdminRequestSync) {
        // pub key is required
        if (ingestPublicKey(chatter->getEncryptor()->getPublicKeyBuffer())) {

            // future enhancement: this would be a good point to generate a message to challenge for a signature. 

            // check if this device id is known
            char knownDeviceId[CHATTER_DEVICE_ID_SIZE+1];
            char knownAlias[CHATTER_ALIAS_NAME_SIZE+1];
            bool trustedKey = chatter->getTrustStore()->findDeviceId ((const uint8_t*)chatter->getEncryptor()->getPublicKeyBuffer(), chatter->getClusterId(), knownDeviceId);
            if(trustedKey) {
                chatter->getTrustStore()->loadAlias(knownDeviceId, knownAlias);
                /*Serial.print("We Know: ");
                Serial.print(knownDeviceId);
                Serial.print("/");
                Serial.println(knownAlias);*/

                // if it's onboard, it should not yet exist in our truststore. if that's the case, do a sync instead
                // if it's sync, we are good to proceed
                return syncDevice(chatter->getClusterId(), knownDeviceId, knownAlias);
            } 
            else {
                // we don't know this key, which is to be expected if it's on onboard
                if (requestType == AdminRequestOnboard) {
                    return onboardNewDevice(chatter->getClusterId(), deviceType, (const uint8_t*)chatter->getEncryptor()->getPublicKeyBuffer());
                }
            }
        }
        else {
            Serial.println("Bad public key!");
        }
    }

    return true;
}

bool BleClusterAdminInterface::sendTxBufferToClient (const char* expectedConfirmation) {
    bool confirmationReceived = false;
    // publish to the tx characteristic,
    tx->writeValue((const char*)txBuffer);

    // wait for the confirmation to appear in the rx characteristic
    unsigned long startTime = millis();
    while (millis() - startTime < maxSessionStepWait && confirmationReceived == false) {
        BLE.poll();

        if (rx->valueLength() >= strlen(expectedConfirmation) && memcmp(rx->value(), expectedConfirmation, strlen(expectedConfirmation)) == 0) {
            confirmationReceived = true;

            // reset the tx buffer for next send
            rx->writeValue("ready");
            status->writeValue("ready");
        }
    }

    return confirmationReceived;
}

bool BleClusterAdminInterface::isConnected () {
    connected = false;
    bleDevice = BLE.central();
    if (bleDevice) {
        Serial.print("Connected to central: ");
        // print the central's MAC address:
        Serial.println(bleDevice.address());    
        connected = true;

        // set the status to that devices mac
        status->writeValue(bleDevice.address().c_str());
        tx->writeValue(bleDevice.address().c_str());

        // this takes over all processing until the connection ends
        while (bleDevice.connected()) {
            if (rx->written()) {
                memset(rxBuffer, 0, BLE_MAX_BUFFER_SIZE+1);
                rxBufferLength = rx->valueLength();
                for (int i = 0; i < rxBufferLength; i++) {
                    rxBuffer[i] = rx->value()[i];
                }

                if (handleClientInput((const char*)rxBuffer, rxBufferLength)) {
                    memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE+1);
                    txBuffer[0] = 'O';
                    txBuffer[1] = 'K';
                    txBuffer[2] = ':';
                    sprintf(((char*)txBuffer+3), "%d.",millis());
                    tx->writeValue((const char*)txBuffer);
                }
                else {
                    tx->writeValue("ERROR");
                }
            }
            delay(100);
        }

        // ready for new connections
        status->writeValue("ready");
    }
    return connected;
}

bool BleClusterAdminInterface::ingestPublicKey (byte* buffer) {
    int bytesRead = 0;
    bool validKey = true; // assume valid until we find a bad byte
    unsigned long startTime = millis();

    // let central device know to produce the pub key
    rx->writeValue("ready");

    tx->writeValue("PUB");
    status->writeValue("PUB");

    // pub key can only be hex with no spaces
    memset(hexEncodedPubKey, 0, ENC_PUB_KEY_SIZE * 2 + 1);
    int keyBytesRead = 0;
    while ((millis() - startTime < maxSessionStepWait) && keyBytesRead < ENC_PUB_KEY_SIZE * 2 && validKey) {
        BLE.poll();
        if (rx->written()) {
            memset(rxBuffer, 0, BLE_MAX_BUFFER_SIZE+1);
            bytesRead = rx->readValue(rxBuffer, BLE_MAX_BUFFER_SIZE);

            Serial.print("pub key read "); Serial.print(bytesRead); Serial.print(" bytes: "); Serial.println((char*)rxBuffer);

            if (memcmp(rxBuffer, "PUB:", 4) == 0) {
                Serial.println("part of pub key receive");
                memcpy(hexEncodedPubKey + keyBytesRead, rxBuffer + 4, bytesRead - 4);
                keyBytesRead += bytesRead - 4;

                for (uint8_t i = 4; i < bytesRead; i++) {
                    char currChar = rxBuffer[i];
                    if ((currChar >= '0' && currChar <= '9') || (currChar >= 'A' && currChar <= 'Z')) {
                        // this looks good;
                    }
                    else {
                        Serial.print("Bad pub key character : "); Serial.println(currChar);
                        validKey = false;
                    }
                }
            }
            else {
                Serial.print("Val read, not pub key: "); Serial.println((const char*)rxBuffer);
            }

            // notify client to write more
            tx->writeValue(String(bytesRead).c_str());
        }
        delay(100);
    }

    Serial.print("FULL KEy: ");
    for (int i = 0; i < keyBytesRead; i++) {
        Serial.print(hexEncodedPubKey[i]);
    }
    Serial.println("");

    if (keyBytesRead == ENC_PUB_KEY_SIZE * 2 && validKey) {
        return true;
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
            for (uint8_t i = 0; i < ENC_PUB_KEY_SIZE*2; i++) {
                Serial.print(encryptor->getHexBuffer()[i]);
            }
            encryptor->clearHexBuffer(); // dont leave key in hex buffer

            memcpy(txBuffer, 0, BLE_MAX_BUFFER_SIZE+1);
            sprintf((char*)txBuffer, "%s%s%s%s%s", CLUSTER_CFG_TRUST, CLUSTER_CFG_DELIMITER, otherDeviceId, (char*)encryptor->getPublicKeyBuffer(), otherDeviceAlias);

            if (sendTxBufferToClient(CLUSTER_CFG_TRUST)) {
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
    memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE+1);
    memcpy(txBuffer, CLUSTER_CFG_KEY, strlen(CLUSTER_CFG_KEY));
    bufferPos += strlen(CLUSTER_CFG_KEY);
    txBuffer[bufferPos++] = CLUSTER_CFG_DELIMITER;
    memcpy(txBuffer + bufferPos, encryptor->getHexBuffer(), ENC_SYMMETRIC_KEY_SIZE*2);

    if (sendTxBufferToClient(CLUSTER_CFG_KEY)) {
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
    memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE+1);
    memcpy(txBuffer, CLUSTER_CFG_IV, strlen(CLUSTER_CFG_IV));
    bufferPos += strlen(CLUSTER_CFG_IV);
    txBuffer[bufferPos++] = CLUSTER_CFG_DELIMITER;
    memcpy(txBuffer + bufferPos, encryptor->getHexBuffer(), ENC_IV_SIZE*2);
    encryptor->clearHexBuffer(); // dont leave key in hex buffer

    if (sendTxBufferToClient(CLUSTER_CFG_IV)) {
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

    memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
    sprintf((char*)txBuffer, "%s%s%s", CLUSTER_CFG_WIFI_SSID, CLUSTER_CFG_DELIMITER, wifiSsid);
    if(sendTxBufferToClient(CLUSTER_CFG_WIFI_SSID)) {
        logConsole("Wifi SSID sent");

        memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
        sprintf((char*)txBuffer, "%s%s%s", CLUSTER_CFG_WIFI_CRED, CLUSTER_CFG_DELIMITER, wifiCred);
        if(sendTxBufferToClient(CLUSTER_CFG_WIFI_CRED)) {
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
    memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
    sprintf((char*)txBuffer, "%01f", CLUSTER_CFG_FREQ, CLUSTER_CFG_DELIMITER, chatter->getClusterStore()->getFrequency(hostClusterId));
    if (sendTxBufferToClient(CLUSTER_CFG_FREQ)) {
        logConsole("Lora freq sent");
    }
    else {
        logConsole("Lora freq not sent");
        return false;
    }
    return true;
}

bool BleClusterAdminInterface::dumpChannels (const char* hostClusterId) {
    memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
    sprintf((char*)txBuffer, "%s%s%s", CLUSTER_CFG_PRIMARY, CLUSTER_CFG_DELIMITER, (char)chatter->getClusterStore()->getPreferredChannel(hostClusterId));

    if (sendTxBufferToClient(CLUSTER_CFG_PRIMARY)) {
        logConsole("Primary channel sent");

        memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
        sprintf((char*)txBuffer, "%s%s%s", CLUSTER_CFG_SECONDARY, CLUSTER_CFG_DELIMITER, (char)chatter->getClusterStore()->getSecondaryChannel(hostClusterId));

        if (sendTxBufferToClient(CLUSTER_CFG_SECONDARY)) {
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
    memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
    sprintf((char*)txBuffer, "%s%s%s", CLUSTER_CFG_AUTH, CLUSTER_CFG_DELIMITER, (char)chatter->getClusterStore()->getAuthType(hostClusterId));

    if (sendTxBufferToClient(CLUSTER_CFG_AUTH)) {
        logConsole("Auth type sent");
    }
    else {
        logConsole("Auth type not sent");
        return false;
    }

    return true;
}


bool BleClusterAdminInterface::dumpTime () {
    memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
    sprintf((char*)txBuffer, "%s%s%s", CLUSTER_CFG_TIME, CLUSTER_CFG_DELIMITER, chatter->getRtc()->getSortableTime());

    if (sendTxBufferToClient(CLUSTER_CFG_TIME)) {
        logConsole("Time sent");
    }
    else {
        logConsole("Time not sent");
        return false;
    }

    return true;
}

bool BleClusterAdminInterface::dumpDevice (const char* deviceId, const char* alias) {
    memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
    sprintf((char*)txBuffer, "%s%s%s%s", CLUSTER_CFG_DEVICE, CLUSTER_CFG_DELIMITER, CLUSTER_CFG_DEVICE, alias);

    if (sendTxBufferToClient(CLUSTER_CFG_DEVICE)) {
        logConsole("Device ID sent");
    }
    else {
        logConsole("Device ID not sent");
        return false;
    }

    return true;



}
