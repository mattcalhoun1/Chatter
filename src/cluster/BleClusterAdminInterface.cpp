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

        memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
        sprintf((char*)txBuffer, "%s", "ready");
        rx->writeValue(txBuffer, BLE_SMALL_BUFFER_SIZE);
        tx->writeValue(txBuffer, BLE_SMALL_BUFFER_SIZE);

        //rx->subscribe();
        //txRead->subscribe();

        BLE.advertise();

        running = true;
    }
    else {
        Serial.println("Error BLE.begin()");
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
        if (ingestPublicKey(chatter->getEncryptor()->getPublicKeyBuffer(), device)) {

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

bool BleClusterAdminInterface::writeBleBufferWait (BLECharacteristic* bleChar, BLECharacteristic* bleFlagChar, uint8_t* buff, int len) {
    // clear the tx read flag
    bleChar->writeValue(buff, BLE_SMALL_BUFFER_SIZE);
    bleFlagChar->writeValue("0");
    unsigned long startTime = millis();
    bool ack = false;
    char flag[] = {'\0'};

    while (!ack && millis() - startTime < maxSessionStepWait) {
        BLE.poll();
        bleFlagChar->readValue(flag, 1);
        ack = flag[0] == '1';
        if (!ack) {
            delay(100);
        }
    }

    return ack;
}

bool BleClusterAdminInterface::sendBufferToClient (BLECharacteristic* bleChar, BLECharacteristic* bleFlagChar, uint8_t* buff, uint8_t len) {
    uint8_t bytesSent = 0;
    unsigned long startTime = millis();
    memset(bleBuffer, BLE_HEADER_CHAR, BLE_SMALL_BUFFER_SIZE);
    bleBufferLength = BLE_SMALL_BUFFER_SIZE;
    if (!writeBleBufferWait(bleChar, bleFlagChar, bleBuffer, bleBufferLength)) {
        logConsole("Failed to transmit header");
        return false;
    }
    while (bytesSent < len && millis() - startTime < maxSessionStepWait) {
        // copy next portion of txbuffer into blebuffer
        memset(bleBuffer, 0, BLE_SMALL_BUFFER_SIZE);
        if (len - bytesSent < BLE_SMALL_BUFFER_SIZE) {
            bleBufferLength = len - bytesSent;
        }
        else {
            bleBufferLength = BLE_SMALL_BUFFER_SIZE;
        }
        memcpy(bleBuffer, buff + bytesSent, bleBufferLength);
        if (!writeBleBufferWait(bleChar, bleFlagChar, bleBuffer, bleBufferLength)) {
            logConsole("Write ble buffer failed or was not acknowledged");
            return true;
        }
        else {
            bytesSent += bleBufferLength;
        }
    }

    memset(bleBuffer, BLE_FOOTER_CHAR, BLE_SMALL_BUFFER_SIZE);
    bleBufferLength = BLE_SMALL_BUFFER_SIZE;
    if (!writeBleBufferWait(bleChar, bleFlagChar, bleBuffer, bleBufferLength)) {
        logConsole("Failed to transmit footer");
        return false;
    }

    return bytesSent >= len;
}

bool BleClusterAdminInterface::sendTxBufferToClient () {
    return sendBufferToClient(tx, txRead, txBuffer, txBufferLength);
}

bool BleClusterAdminInterface::bleBufferContainsHeader () {
    for (uint8_t i = 0; i < BLE_SMALL_BUFFER_SIZE; i++) {
        if ((char)bleBuffer[i] != BLE_HEADER_CHAR) {
            return false;
        }
    }

    return true;
}

bool BleClusterAdminInterface::bleBufferContainsFooter () {
    for (uint8_t i = 0; i < BLE_SMALL_BUFFER_SIZE; i++) {
        if ((char)bleBuffer[i] != BLE_FOOTER_CHAR) {
            return false;
        }
    }

    return true;
}

uint8_t BleClusterAdminInterface::receiveBufferFromClient (BLECharacteristic* bleChar, BLECharacteristic* bleFlagChar, uint8_t* buff, uint8_t maxLength, BLEDevice* device) {
    bool receivedHeader = false;
    bool receivedFooter = false;
    uint8_t bytesRead = 0;

    unsigned long startTime = millis();
    memset(buff, 0, maxLength);
    char flag[1];

    while ((!receivedHeader || !receivedFooter) && (millis() - startTime < maxSessionStepWait) && (bytesRead <= maxLength)) {
        if(device) {
            BLE.poll();
            bleFlagChar->readValue(flag, 1);

            if (flag[0] == '0') {
                memset(bleBuffer, 0, BLE_SMALL_BUFFER_SIZE);
                bleChar->readValue(bleBuffer, BLE_SMALL_BUFFER_SIZE);

                if (!receivedHeader && bleBufferContainsHeader()) {
                    receivedHeader = true;
                }
                else if (!receivedFooter && bleBufferContainsFooter()) {
                    receivedFooter = true;
                }
                else {
                    // add all chars up to the null/term
                    uint8_t end = findChar('\0', bleBuffer, BLE_SMALL_BUFFER_SIZE);
                    if (end == 255 && bytesRead + BLE_SMALL_BUFFER_SIZE <= maxLength) {
                        // this full buffer is part of the transmission
                        memcpy(buff + bytesRead, bleBuffer, BLE_SMALL_BUFFER_SIZE);
                        bytesRead += BLE_SMALL_BUFFER_SIZE;
                    }
                    else if (bytesRead + (end+1) <= maxLength) {
                        memcpy(buff + bytesRead, bleBuffer, end + 1);
                        bytesRead += (end+1);
                    }
                    else {
                        logConsole("Message too large for buffer!");
                        return 0;
                    }
                }

                // acknowledge receipt to the device
                bleFlagChar->writeValue("1");
                BLE.poll();
            }

        }
        else {
            logConsole("device disconnected while waiting");
            return false;
        }
    }

    return receivedHeader && receivedFooter ? bytesRead : 0;
}


bool BleClusterAdminInterface::receiveRxBufferFromClient (BLEDevice* device) {
    rxBufferLength = receiveBufferFromClient(rx, rxRead, rxBuffer, BLE_MAX_BUFFER_SIZE, device);
    return rxBufferLength > 0;
}


bool BleClusterAdminInterface::isConnected () {
    connected = false;
    bleDevice = BLE.central();
    if (bleDevice && bleDevice.connected()) {
        Serial.print("Connected to central: ");
        // print the central's MAC address:
        Serial.println(bleDevice.address());    
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
            if (receiveRxBufferFromClient(&bleDevice)) {
                if (findChar(CLUSTER_CFG_DELIMITER, rxBuffer, rxBufferLength) != 255) {
                    logConsole("handling client input");
                    if (handleClientInput((const char*)rxBuffer, rxBufferLength, &bleDevice)) {
                        memset(bleBuffer, 0, BLE_SMALL_BUFFER_SIZE);
                        sprintf((char*)bleBuffer, "%s", "OK");
                        tx->writeValue(bleBuffer, BLE_SMALL_BUFFER_SIZE);
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
                memset(bleBuffer, 0, BLE_SMALL_BUFFER_SIZE);
                sprintf((char*)bleBuffer, "%s", "ERROR");
                success = false;
                tx->writeValue(bleBuffer, BLE_SMALL_BUFFER_SIZE);
            }
        }

        logConsole("BLE device no longer connected");

        // ready for new connections
        memset (bleBuffer, 0, BLE_SMALL_BUFFER_SIZE);
        memcpy(bleBuffer, "ready",5);
        status->writeValue(bleBuffer, BLE_SMALL_BUFFER_SIZE);
    }
    return connected;
}

bool BleClusterAdminInterface::ingestPublicKey (byte* buffer, BLEDevice* bleDevice) {
    //int bytesRead = 0;
    bool validKey = true; // assume valid until we find a bad byte
    unsigned long startTime = millis();

    // let central device know to produce the pub key
    //memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
    //memcpy(txBuffer, "ready", 5);
    //rx->writeValue(txBuffer, BLE_MAX_BUFFER_SIZE);

    memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
    memcpy(txBuffer, "PUB", 3);
    txBufferLength = 3;
    //tx->writeValue(txBuffer, BLE_MAX_BUFFER_SIZE);
    //status->writeValue(txBuffer, BLE_MAX_BUFFER_SIZE);
    if (sendTxBufferToClient()) {
        // pub key can only be hex with no spaces
        memset(hexEncodedPubKey, 0, ENC_PUB_KEY_SIZE * 2 + 1);

        // receive key bytes from  client
        if(receiveRxBufferFromClient(bleDevice)) {
            if (memcmp(rxBuffer, "PUB:", 4) == 0) {
                memcpy(hexEncodedPubKey, rxBuffer + 4, rxBufferLength - 4);

                for (uint8_t i = 4; i < rxBufferLength; i++) {
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
                return false;
            }
        }
        else {
            logConsole("Pub key not received");
        }
    }
    else {
        logConsole("pub key request not acknowledged");
    }

    Serial.print("Device Public Key: ");
    for (int i = 0; i < ENC_PUB_KEY_SIZE * 2; i++) {
        Serial.print(hexEncodedPubKey[i]);
    }
    Serial.println("");

    if (rxBufferLength == (ENC_PUB_KEY_SIZE * 2) + 4 && validKey) {
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
            //encryptor->clearHexBuffer(); // dont leave key in hex buffer

            memcpy(txBuffer, 0, BLE_MAX_BUFFER_SIZE+1);
            sprintf((char*)txBuffer, "%s%c%s%s%s", CLUSTER_CFG_TRUST, CLUSTER_CFG_DELIMITER, otherDeviceId, (char*)encryptor->getHexBuffer(), otherDeviceAlias);
            txBufferLength = strlen(CLUSTER_CFG_TRUST) + 1 + CHATTER_DEVICE_ID_SIZE + ENC_PUB_KEY_SIZE*2 + strlen(otherDeviceAlias);

            if (sendTxBufferToClient()) {
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
    txBufferLength = strlen(CLUSTER_CFG_KEY) + 1 + ENC_SYMMETRIC_KEY_SIZE * 2;

    if (sendTxBufferToClient()) {
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
    txBufferLength = strlen(CLUSTER_CFG_IV) + 1 + ENC_IV_SIZE*2;

    if (sendTxBufferToClient()) {
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
    sprintf((char*)txBuffer, "%s%c%s", CLUSTER_CFG_WIFI_SSID, CLUSTER_CFG_DELIMITER, wifiSsid);
    txBufferLength = strlen(CLUSTER_CFG_WIFI_SSID) + 1 + strlen(wifiSsid);
    if(sendTxBufferToClient()) {
        logConsole("Wifi SSID sent");

        memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
        txBufferLength = strlen(CLUSTER_CFG_WIFI_CRED) + 1 + strlen(wifiCred);
        sprintf((char*)txBuffer, "%s%c%s", CLUSTER_CFG_WIFI_CRED, CLUSTER_CFG_DELIMITER, wifiCred);
        if(sendTxBufferToClient()) {
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
    sprintf((char*)txBuffer, "%s%c%01f", CLUSTER_CFG_FREQ, CLUSTER_CFG_DELIMITER, chatter->getClusterStore()->getFrequency(hostClusterId));
    txBufferLength = strlen(CLUSTER_CFG_FREQ) + 1 + 5;
    if (sendTxBufferToClient()) {
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
    sprintf((char*)txBuffer, "%s%c%s", CLUSTER_CFG_PRIMARY, CLUSTER_CFG_DELIMITER, (char)chatter->getClusterStore()->getPreferredChannel(hostClusterId));
    txBufferLength = strlen(CLUSTER_CFG_PRIMARY) + 1 + 1;

    if (sendTxBufferToClient()) {
        logConsole("Primary channel sent");

        memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
        sprintf((char*)txBuffer, "%s%c%s", CLUSTER_CFG_SECONDARY, CLUSTER_CFG_DELIMITER, (char)chatter->getClusterStore()->getSecondaryChannel(hostClusterId));
        txBufferLength = strlen(CLUSTER_CFG_SECONDARY) + 1 + 1;
        if (sendTxBufferToClient()) {
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
    sprintf((char*)txBuffer, "%s%c%s", CLUSTER_CFG_AUTH, CLUSTER_CFG_DELIMITER, (char)chatter->getClusterStore()->getAuthType(hostClusterId));
    txBufferLength = strlen(CLUSTER_CFG_AUTH) + 1 + 1;

    if (sendTxBufferToClient()) {
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
    sprintf((char*)txBuffer, "%s%c%s", CLUSTER_CFG_TIME, CLUSTER_CFG_DELIMITER, chatter->getRtc()->getSortableTime());
    txBufferLength = strlen(CLUSTER_CFG_TIME) + 1 + 12;

    if (sendTxBufferToClient()) {
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
    sprintf((char*)txBuffer, "%s%c%s%s", CLUSTER_CFG_DEVICE, CLUSTER_CFG_DELIMITER, deviceId, alias);
    txBufferLength = strlen(CLUSTER_CFG_DEVICE) + 1 + CHATTER_DEVICE_ID_SIZE + strlen(alias);

    if (sendTxBufferToClient()) {
        logConsole("Device ID sent");
    }
    else {
        logConsole("Device ID not sent");
        return false;
    }

    return true;
}

bool BleClusterAdminInterface::dumpLicense (const char* deviceId) {
    if (generateEncodedLicense(deviceId)) {
        memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE);
        sprintf((char*)txBuffer, "%s%c%s%s", CLUSTER_CFG_LICENSE, CLUSTER_CFG_DELIMITER, chatter->getDeviceId(), hexEncodedLicense);
        txBufferLength = strlen(CLUSTER_CFG_LICENSE) + 1 + CHATTER_DEVICE_ID_SIZE + (ENC_SIGNATURE_SIZE * 2);

        if (sendTxBufferToClient()) {
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


uint8_t BleClusterAdminInterface::findChar (char toFind, const  uint8_t* buffer, uint8_t bufferLen) {
    for (uint8_t i = 0; i < bufferLen; i++) {
        Serial.print((char)buffer[i]);
        if (buffer[i] == (uint8_t)toFind) {
            return i;
        }
    }
    Serial.println("");

    return 255; // not found
}